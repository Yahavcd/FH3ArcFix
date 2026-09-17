#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>

#include <array>
#include <atomic>
#include <cwchar>
#include <iterator>
#include <mutex>
#include <string>

namespace {

HMODULE g_realD3D12 = nullptr;
std::once_flag g_realLoadOnce;
std::mutex g_patchMutex;

using PFN_REAL_D3D12_CREATE_DEVICE = HRESULT (WINAPI *)(IUnknown*, D3D_FEATURE_LEVEL, REFIID, void**);
using PFN_REAL_D3D12_SERIALIZE_ROOT_SIGNATURE = HRESULT (WINAPI *)(
    const D3D12_ROOT_SIGNATURE_DESC*, D3D_ROOT_SIGNATURE_VERSION, ID3DBlob**, ID3DBlob**);
using PFN_CREATE_DEPTH_STENCIL_VIEW = void (STDMETHODCALLTYPE *)(
    ID3D12Device*, ID3D12Resource*, const D3D12_DEPTH_STENCIL_VIEW_DESC*, D3D12_CPU_DESCRIPTOR_HANDLE);

PFN_REAL_D3D12_CREATE_DEVICE g_realCreateDevice = nullptr;
PFN_REAL_D3D12_SERIALIZE_ROOT_SIGNATURE g_realSerializeRootSignature = nullptr;

// ID3D12Device::CreateDepthStencilView is vtable slot 21:
//   0-2   IUnknown
//   3-6   ID3D12Object
//   7-20  preceding ID3D12Device methods
//   21    CreateDepthStencilView
//
// Newer ID3D12DeviceN interfaces extend the base interface by appending methods;
// they do not reorder the ID3D12Device ABI. We also explicitly QueryInterface
// for ID3D12Device before patching.
constexpr size_t VT_CREATE_DEPTH_STENCIL_VIEW = 21;
constexpr size_t MAX_HOOK_RECORDS = 16;

static_assert(sizeof(void*) == 8, "FH3ArcFix supports x64 only.");
static_assert(std::atomic<void**>::is_always_lock_free,
    "FH3ArcFix requires lock-free pointer atomics on x64.");

// A record is written once and never removed. 'original' is deliberately not
// atomic: publishing vtable with release semantics safely publishes the prior
// original-function write. Readers first acquire-load vtable, and only read
// original after observing the matching published vtable.
struct HookRecord {
    PFN_CREATE_DEPTH_STENCIL_VIEW original = nullptr;
    std::atomic<void**> vtable{nullptr};
};

std::array<HookRecord, MAX_HOOK_RECORDS> g_hookRecords{};

#ifdef FH3ARCFIX_DIAGNOSTICS
void Trace(const char* text) {
    OutputDebugStringA(text);
}
#else
void Trace(const char*) {}
#endif

bool IsExecutableAddress(const void* address) {
    if (!address) return false;

    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(address, &mbi, sizeof(mbi)) != sizeof(mbi)) {
        return false;
    }

    if (mbi.State != MEM_COMMIT ||
        (mbi.Protect & PAGE_GUARD) != 0 ||
        (mbi.Protect & PAGE_NOACCESS) != 0) {
        return false;
    }

    switch (mbi.Protect & 0xFFu) {
        case PAGE_EXECUTE:
        case PAGE_EXECUTE_READ:
        case PAGE_EXECUTE_READWRITE:
        case PAGE_EXECUTE_WRITECOPY:
            return true;
        default:
            return false;
    }
}

bool IsIntelArcAdapter(IUnknown* adapter) {
    if (!adapter) return false;

    IDXGIAdapter1* dxgiAdapter = nullptr;
    if (FAILED(adapter->QueryInterface(__uuidof(IDXGIAdapter1), reinterpret_cast<void**>(&dxgiAdapter))) || !dxgiAdapter) {
        return false;
    }

    DXGI_ADAPTER_DESC1 desc{};
    const HRESULT hr = dxgiAdapter->GetDesc1(&desc);
    dxgiAdapter->Release();
    if (FAILED(hr)) return false;

    // Intel PCI vendor ID and an Arc-family adapter description.
    return desc.VendorId == 0x8086 && std::wcsstr(desc.Description, L"Arc") != nullptr;
}

PFN_CREATE_DEPTH_STENCIL_VIEW FindOriginalCreateDsv(ID3D12Device* self) {
    if (!self) return nullptr;

    void** vtable = *reinterpret_cast<void***>(self);
    if (!vtable) return nullptr;

    // Hot path: no mutex and no allocation. A successful acquire-load of the
    // published vtable synchronizes with PatchDeviceVtable's release-store and
    // makes the immutable 'original' pointer visible to this thread.
    for (const auto& record : g_hookRecords) {
        if (record.vtable.load(std::memory_order_acquire) == vtable) {
            return record.original;
        }
    }

    return nullptr;
}

void STDMETHODCALLTYPE HookCreateDepthStencilView(
    ID3D12Device* self,
    ID3D12Resource* resource,
    const D3D12_DEPTH_STENCIL_VIEW_DESC* desc,
    D3D12_CPU_DESCRIPTOR_HANDLE dest) {

    auto original = FindOriginalCreateDsv(self);
    if (!original) {
        // Fail closed: if our bookkeeping is unavailable, do not fabricate a
        // call into an unknown function pointer.
        Trace("[FH3ArcFix] missing hook record; CreateDSV not forwarded\r\n");
        return;
    }

    const D3D12_DEPTH_STENCIL_VIEW_DESC* forwarded = desc;
    D3D12_DEPTH_STENCIL_VIEW_DESC fixed{};

    // FH3 1.0.125.2 creates one null Texture2D DSV with UNKNOWN format.
    // With no resource, UNKNOWN has no resource format to inherit and the
    // D3D12 runtime removes the device with DXGI_ERROR_INVALID_CALL on Arc.
    // D32_FLOAT is a legal null DSV format and is the minimal tested fix.
    if (resource == nullptr &&
        desc != nullptr &&
        desc->Format == DXGI_FORMAT_UNKNOWN &&
        desc->ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D &&
        desc->Flags == D3D12_DSV_FLAG_NONE &&
        desc->Texture2D.MipSlice == 0) {

        fixed = *desc;
        fixed.Format = DXGI_FORMAT_D32_FLOAT;
        forwarded = &fixed;
        Trace("[FH3ArcFix] applied null-DSV format fix\r\n");
    }

    original(self, resource, forwarded, dest);
}

bool PatchDeviceVtable(ID3D12Device* device) {
    if (!device) return false;

    void** vtable = *reinterpret_cast<void***>(device);
    if (!vtable) return false;

    // Setup is rare and may use a mutex. Only CreateDepthStencilView's hot
    // read path is lock-free.
    std::lock_guard<std::mutex> lock(g_patchMutex);

    // Already registered/patched.
    for (const auto& record : g_hookRecords) {
        if (record.vtable.load(std::memory_order_acquire) == vtable) {
            return true;
        }
    }

    void* originalAddress = vtable[VT_CREATE_DEPTH_STENCIL_VIEW];
    auto original = reinterpret_cast<PFN_CREATE_DEPTH_STENCIL_VIEW>(originalAddress);

    // Runtime sanity checks before modifying the shared runtime vtable.
    if (!originalAddress || original == &HookCreateDepthStencilView ||
        !IsExecutableAddress(originalAddress)) {
        Trace("[FH3ArcFix] refused vtable patch: invalid CreateDSV slot\r\n");
        return false;
    }

    HookRecord* emptyRecord = nullptr;
    for (auto& record : g_hookRecords) {
        if (record.vtable.load(std::memory_order_relaxed) == nullptr) {
            emptyRecord = &record;
            break;
        }
    }

    // FH3 is expected to use a single D3D12 runtime vtable. The bounded table
    // deliberately fails closed if an unexpected process creates more than 16
    // distinct vtables; records are never recycled because patched vtables may
    // remain reachable for the lifetime of the process.
    if (!emptyRecord) {
        Trace("[FH3ArcFix] hook record table full; vtable not patched\r\n");
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(&vtable[VT_CREATE_DEPTH_STENCIL_VIEW], sizeof(void*),
                        PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Trace("[FH3ArcFix] VirtualProtect failed; vtable not patched\r\n");
        return false;
    }

    // Publish bookkeeping before exposing the hook through the runtime vtable.
    // The release-store makes the preceding 'original' write visible to a hook
    // thread that observes this vtable via an acquire-load.
    emptyRecord->original = original;
    emptyRecord->vtable.store(vtable, std::memory_order_release);

    // InterlockedExchangePointer is the Windows-supported atomic pointer swap
    // used to expose the hook after its record has been published.
    InterlockedExchangePointer(
        reinterpret_cast<PVOID volatile*>(&vtable[VT_CREATE_DEPTH_STENCIL_VIEW]),
        reinterpret_cast<PVOID>(&HookCreateDepthStencilView));

    DWORD ignored = 0;
    if (!VirtualProtect(&vtable[VT_CREATE_DEPTH_STENCIL_VIEW], sizeof(void*),
                        oldProtect, &ignored)) {
        Trace("[FH3ArcFix] warning: failed to restore vtable page protection\r\n");
    }

    FlushInstructionCache(GetCurrentProcess(),
                          &vtable[VT_CREATE_DEPTH_STENCIL_VIEW], sizeof(void*));
    Trace("[FH3ArcFix] patched ID3D12Device::CreateDepthStencilView\r\n");
    return true;
}

void LoadRealD3D12() {
    std::call_once(g_realLoadOnce, [] {
        wchar_t systemDir[MAX_PATH]{};
        const UINT n = GetSystemDirectoryW(systemDir, static_cast<UINT>(std::size(systemDir)));
        if (!n || n >= std::size(systemDir)) return;

        std::wstring path = systemDir;
        path += L"\\d3d12.dll";
        g_realD3D12 = LoadLibraryW(path.c_str());
        if (!g_realD3D12) return;

        g_realCreateDevice = reinterpret_cast<PFN_REAL_D3D12_CREATE_DEVICE>(
            GetProcAddress(g_realD3D12, "D3D12CreateDevice"));
        g_realSerializeRootSignature = reinterpret_cast<PFN_REAL_D3D12_SERIALIZE_ROOT_SIGNATURE>(
            GetProcAddress(g_realD3D12, "D3D12SerializeRootSignature"));
    });
}

} // namespace

extern "C" HRESULT WINAPI FH3ArcFix_D3D12CreateDevice(
    IUnknown* adapter,
    D3D_FEATURE_LEVEL minimumFeatureLevel,
    REFIID riid,
    void** ppDevice) {

    LoadRealD3D12();
    if (!g_realCreateDevice) return E_NOINTERFACE;

    const bool targetArc = IsIntelArcAdapter(adapter);
    const HRESULT hr = g_realCreateDevice(adapter, minimumFeatureLevel, riid, ppDevice);

    if (targetArc && SUCCEEDED(hr) && ppDevice && *ppDevice) {
        ID3D12Device* baseDevice = nullptr;
        IUnknown* returned = reinterpret_cast<IUnknown*>(*ppDevice);
        if (SUCCEEDED(returned->QueryInterface(__uuidof(ID3D12Device), reinterpret_cast<void**>(&baseDevice))) && baseDevice) {
            PatchDeviceVtable(baseDevice);
            baseDevice->Release();
        }
    }

    return hr;
}

extern "C" HRESULT WINAPI FH3ArcFix_D3D12SerializeRootSignature(
    const D3D12_ROOT_SIGNATURE_DESC* rootSignature,
    D3D_ROOT_SIGNATURE_VERSION version,
    ID3DBlob** blob,
    ID3DBlob** errorBlob) {

    LoadRealD3D12();
    if (!g_realSerializeRootSignature) return E_NOINTERFACE;
    return g_realSerializeRootSignature(rootSignature, version, blob, errorBlob);
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(instance);
        Trace("[FH3ArcFix] loaded\r\n");
    }
    return TRUE;
}
