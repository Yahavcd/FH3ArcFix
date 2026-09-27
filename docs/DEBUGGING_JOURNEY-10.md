# FH3ArcFix: How I found the fix

I wasn't planning to write a compatibility patch. I just wanted to play Forza Horizon 3, but it would start loading and then close without telling me why.

I'm not a DirectX expert. I worked through this with ChatGPT, learning which tools to use and what the errors meant as we went. I ran the tests on my machine and brought back the results. That's what I mean by "we" below.

I'd already tried the usual launch fixes before starting the chat. Later we tried to get around D3D12 by using Vulkan. When that didn't work, we went back to native D3D12 to find out what was actually crashing. Even after finding the bad call, our first serious attempt at a fix introduced another crash.

This is how that happened, including how I used ChatGPT along the way. The implementation details are in [`TECHNICAL.md`](TECHNICAL.md). I've grouped the tests by what we were trying to figure out, so this isn't a log of every launch in exact order.

## The game just closed

My system was running FH3 build `1.0.125.2` on an Intel Arc Pro B50, with Windows 11 24H2.

At first, all I had was a game that wouldn't stay open. There was no useful error on screen and no stack trace to point me toward graphics initialization.

Before I started working with ChatGPT, I had already tried the usual FH3 suggestions on my own: an antivirus exclusion, disabling microphone access for the game, and disabling overlays. None of them fixed it. That didn't rule out every possible startup issue, but these particular fixes hadn't helped. I still didn't know whether it was the game, the driver, or something else on the machine.

The next step was to get an actual error to work with. We used PowerShell's `Get-WinEvent` to check the Windows Application log for crash records tied to the executable and package:

```text
forza_x64_release_final.exe
Microsoft.OpusPG
```

Windows had recorded things the game hadn't shown me: the faulting application or module, exception code, fault offset, and crash-report details. `MoAppCrash` came from those records too. It wasn't an error message I had seen in the game.

The native crash signature we started comparing against was:

```text
0xc0000005
forza_x64_release_final.exe + 0x1f28dec
```

Now, after changing something, we could check whether it still failed at the same place. That was more useful than just saying it closed again.

We still didn't know what caused the access violation. Later, we found that it happened after an earlier graphics failure. At this stage we were only reading Windows crash records. Full dumps, WinDbg, and D3D12 validation came later.

## Why I wanted to try Vulkan

After the usual fixes failed, we found reports of similar FH3 crashes on Intel Arc. That made the native D3D12 path look more suspicious, even though we still had no proof the driver was actually the problem.

That seemed weird to me. FH3 is already a DX12 game. I expected Arc to handle that better than something using an older API like DX11. So why was it crashing before I could even play?

I didn't need to understand the whole problem at that point. Getting the game running would have been enough. That's why Vulkan seemed worth trying. Maybe we could avoid the native D3D12 problem altogether:

```text
FH3's D3D12 calls
        ↓
VKD3D-Proton / translation layer
        ↓
Vulkan
        ↓
Intel Vulkan driver
```

I was still assuming the native graphics path might be the problem. We hadn't proved that the driver was at fault, and I wasn't thinking about writing a patch for the game. I was looking for a workaround.

## The wrapper experiments

We tried several combinations of wrappers, versions, local DLLs, and settings. It wasn't just one failed Vulkan attempt before moving on.

One early combination was VKD3D-Proton `3.0.1` with DXVK `2.7.1`. The DLLs we were changing included:

```text
d3d12.dll
d3d12core.dll
dxgi.dll
```

We also changed the VKD3D/DXVK configuration and logging environment. The variable names are in the appendix. They were part of the experiments, not a set of settings that eventually fixed the game.

There were a few ideas behind these tests. Maybe another wrapper version would work. Maybe the GPU identity the game saw was making it choose a bad startup path. We tried an AMD/RX 480-like identity and an older combination of VKD3D `2.8` with DXVK `2.0`. FH3 was an early D3D12 game, so trying an older stack seemed worth a shot.

DXVK `2.1` also came up as an option in the chat, but I don't have a confirmed test result for that version.

Some of this did change the behavior. The splash could stay up longer, the game could hang instead of closing, or it could crash somewhere else. One offset recorded during the wrapper tests was:

```text
forza_x64_release_final.exe + 0x21704f2
```

That kept us looking into it, but we couldn't tell what the change meant. Had we got past the original bug? Had the wrapper introduced a different one? Had changing the GPU identity changed which part of startup the game reached? A longer splash didn't answer those questions.

UWP/package loading added another complication. In some tests, we weren't sure the intended local graphics DLLs had actually loaded. That made those results harder to interpret, but it wasn't the main reason we gave up on this direction.

The bigger issue was that none of the combinations gave us a reliable way to run the game. We kept changing things and getting different failures without understanding the original one. Another wrapper version was always something we could try, but we weren't getting much further.

## Back to native D3D12

Eventually I wanted to stop swapping wrappers and find out what was actually broken in the original path.

So we went back to native D3D12. This wasn't proof that Vulkan could never work. It just wasn't giving us a working game or a clear explanation, and I wanted to debug the game without another compatibility layer in the way.

We disabled or renamed the local translation DLLs, using names like:

```text
d3d12.dll.VKD3D-OFF
d3d12core.dll.VKD3D-OFF
dxgi.dll.VKD3D-OFF
```

We removed the VKD3D/DXVK environment variables too. Leaving half the experimental setup active would have made it harder to know what we were testing.

The original crash came back:

```text
0xc0000005
forza_x64_release_final.exe + 0x1f28dec
```

The game still didn't work, but we had the same failure to investigate again.

We also tried disabling `TargetHardwareProfiler.dll`, the game's hardware-profiling component. That moved the crash to approximately:

```text
forza_x64_release_final.exe + 0x1d006c2
```

It looked like another possible lead, but the game still crashed. Changing the address didn't prove the profiler was the cause, so we restored the DLL. Like the wrapper tests, this had changed the failure without giving us a fix.

At this point we needed more than another crash address. We needed to see what happened before the game reached it.

## Looking at the crash in WinDbg

The Windows records told us where the process finally failed, but not enough about how it got there. We moved on to full crash dumps and WinDbg to inspect thread stacks, loaded modules, and the code around the game's error handling.

That showed FH3 had entered its own fatal error path, associated with this string:

```text
Video card
```

In that path, the game called:

```text
ID3D12Device::GetDeviceRemovedReason()
```

The result was:

```text
0x887A0001
DXGI_ERROR_INVALID_CALL
```

So the access violation wasn't the first thing going wrong. The graphics device had already failed, and FH3 was crashing after entering its fatal error handling.

This was something the earlier changes in crash address hadn't told us. Now we had evidence of a failure before the `0xc0000005` we had been comparing between launches.

But `DXGI_ERROR_INVALID_CALL` still didn't tell us which call was invalid or what the game had passed to it. That was what we needed to find next.

## Using the D3D12 Debug Layer

We installed the Windows graphics debugging components:

```powershell
Add-WindowsCapability -Online -Name "Tools.Graphics.DirectX~~~~0.0.1.0"
```

Then added the executable to `d3dconfig`:

```text
d3dconfig apps --add forza_x64_release_final.exe
```

We enabled the D3D12 Debug Layer for FH3 and also set up DRED while looking into the device removal. The useful result came from the Debug Layer and what we inspected in WinDbg. DRED was part of the setup, but it wasn't what showed us the bad descriptor.

We also used DebugView to watch diagnostic output. Later, it helped us see messages from the prototype through `OutputDebugString`.

There was quite a bit of output to sort through. We saw messages about `GetGPUDescriptorHandleForHeapStart` on a heap that wasn't shader-visible, repeated sampler warnings, an invalid depth-stencil-view format, and errors following the device failure.

We couldn't just pick the first warning and assume it was the cause. The invalid `CreateDepthStencilView` call became the one to investigate because it connected to the device-removal failure.

To catch it while it happened, we configured a debugger break on D3D12 Message ID `47`:

```text
d3dconfig message-break allow-debug-breaks=true
d3dconfig message-break --add-id-12 47
```

This let us stop while D3D12 was checking the call, before the game reached its fatal path and crashed. The relevant stack included frames equivalent to:

```text
DXGIDebug
D3D12SDKLayers validation
CCreateDepthStencilViewValidator::Validate
NDebug::CDevice::CreateDepthStencilView
FH3 call site
```

We now had a specific function to inspect:

```text
ID3D12Device::CreateDepthStencilView
```

The next step was to look at what FH3 had passed to it.

## What was wrong with the call

At the validation break, the arguments showed:

```text
pResource = nullptr
pDesc     = 0000002e`764fe040
```

That address is from the captured session. It isn't a fixed address to use in another run. It gave us a place to read the descriptor while the rejected call was still there.

In that session, this WinDbg command:

```text
dd 0000002e`764fe040 L8
```

returned:

```text
0000002e`764fe040  00000000 00000003 00000000 00000000
0000002e`764fe050  00000000 00000000 00000001 00000000
```

The relevant fields of `D3D12_DEPTH_STENCIL_VIEW_DESC` were:

```text
Format         = 0  → DXGI_FORMAT_UNKNOWN
ViewDimension  = 3  → D3D12_DSV_DIMENSION_TEXTURE2D
Flags          = 0  → D3D12_DSV_FLAG_NONE
MipSlice       = 0
```

So, in simplified form, the game was doing this:

```text
CreateDepthStencilView(
    pResource = nullptr,
    pDesc->Format = DXGI_FORMAT_UNKNOWN,
    pDesc->ViewDimension = TEXTURE2D,
    ...
)
```

A null depth-stencil view wasn't the problem on its own. It was the null resource together with `DXGI_FORMAT_UNKNOWN`. With a backing resource, the view can get its format from that resource. Here there was nothing to get it from.

The validation error was:

```text
CREATEDEPTHSTENCILVIEW_INVALIDFORMAT
```

That gave us a much clearer explanation than "FH3 doesn't work on Arc." We could see invalid input from the game at the point where D3D12 rejected it.

I'd started out suspecting Intel's driver. Now we had an invalid call from FH3 to test. That didn't tell us how every other driver handled the same call, and we still had to find out whether correcting it would actually let the game run.

## Trying to fix the call

The first correction to try was to keep the resource null, leave the view dimension, flags, and mip slice alone, and change the format:

```text
DXGI_FORMAT_UNKNOWN → DXGI_FORMAT_D32_FLOAT
```

There was no backing texture in this call, so we weren't converting the game's real depth buffers to floating point. We were giving this one null descriptor a valid depth format. Normal depth-stencil views would stay as they were.

That also kept the test small. We didn't need to start by creating a dummy depth resource, changing pipeline-state formats, editing the executable, or replacing the renderer. We could first check whether correcting this input stopped the device failure.

We used a local `d3d12.dll` proxy for that. It would load the real system D3D12 runtime, forward the exported entry points FH3 needed, obtain the device, and intercept `ID3D12Device::CreateDepthStencilView`.

We were using a local graphics DLL again, but this time it would keep the game on native D3D12 and change one specific call. There was no Vulkan translation involved.

The first serious implementation, v0.2, cloned the COM vtable. It copied 44 entries, which wasn't a safe assumption for the runtime object and interfaces involved.

That introduced new crashes of its own. One run showed an access violation in `ucrtbase.dll` with `0xc0000005`, and another ended with `0xc0000409`.

Now our own hook was breaking the process. We couldn't take that failed launch as evidence that the descriptor correction was wrong, because the code meant to test it had a separate problem.

So we dropped the vtable-clone approach. We still wanted to test the same format change, but needed a hook that didn't introduce another crash.

## The version that worked

In v0.3, we patched the existing `CreateDepthStencilView` entry in place. It's slot `21` on the base `ID3D12Device` interface. The other entries stayed in the runtime's original vtable.

DebugView and the prototype's output let us check that the proxy had loaded, the hook was installed, and the matching call was corrected. The output looked roughly like this:

```text
[FH3ArcFix] proxy loaded
D3D12CreateDevice -> hr=00000000
PATCHED IN-PLACE
FIX APPLIED: null DSV UNKNOWN -> D32_FLOAT
after fixed CreateDSV GetDeviceRemovedReason=0x00000000
```

The `FIX APPLIED` line told us the hook had made the change. The device check told us whether that change helped. After the corrected call, `GetDeviceRemovedReason` returned `0x00000000` instead of `0x887A0001` / `DXGI_ERROR_INVALID_CALL`. Initialization got past the point that had been failing.

Then I could actually play. FH3 continued through startup and into normal gameplay on the B50. After all the tests that just moved the crash or made the splash hang longer, this one let the game run.

That was the working fix: keep native D3D12 and correct the invalid null DSV when the game creates it.

### What the released fix checks

The released fix checks for this exact pattern:

```text
resource == nullptr
Format == DXGI_FORMAT_UNKNOWN
ViewDimension == D3D12_DSV_DIMENSION_TEXTURE2D
Flags == D3D12_DSV_FLAG_NONE
MipSlice == 0
```

Only when all of those match does it copy the descriptor, change the copy's format to `DXGI_FORMAT_D32_FLOAT`, and call the original function with that copy. Every other `CreateDepthStencilView` call goes through unchanged.

These are the checks in the released code. I'm not listing them as the exact contents of every earlier prototype.

## Other issues and later builds

Getting past the startup crash didn't make every other issue disappear.

The game could still show:

```text
FH204: Unsupported graphics card detected
```

Choosing `Ignore and continue` let me play. Suppressing that warning wasn't part of the initial release because it was a separate hardware-detection issue, not the invalid D3D12 call we had fixed.

`Frame Smoothing` caused a severe performance drop on the tested Arc system. We documented that separately too. There was also some minor pop-in and lighting or shadow flicker, but we didn't establish whether those were Arc-specific or just behavior of the game itself. I wouldn't assume the startup bug explained them.

The initial standard release checked for Intel vendor ID `0x8086` and an adapter description containing `Arc`. The investigation and gameplay testing had been on my Arc Pro B50, so the release was limited to that hardware family. That didn't mean only Arc could ever be affected by the invalid call.

A later experimental all-GPUs build removed the vendor/adapter check and kept the same exact null-DSV correction. That let people test it on other Intel GPUs, AMD, NVIDIA, Moore Threads, other Chinese GPUs, and less common D3D12 implementations. Removing the check didn't mean those configurations had all been tested or confirmed to work.

There was also a later v1.0.1 hardening pass for hook bookkeeping, synchronization, and validation of the target entry. Those details are in the appendix. They improved the implementation without changing the compatibility rule or explaining how we originally found the bug.

## How I worked with ChatGPT

In [issue #1](https://github.com/Yahavcd/FH3ArcFix/issues/1#issuecomment-5845402880), someone asked how I worked with ChatGPT to find the fix. They were interested in using a similar approach for other problems, not just FH3.

As I said in [my reply](https://github.com/Yahavcd/FH3ArcFix/issues/1#issuecomment-5846930411), I didn't really start with a workflow. Most of this was me slowly learning how to investigate it. This section is my summary of that back-and-forth, not a transcript of the chat.

GPT helped explain which tools to use, what the errors meant, and what we could check next. I ran the tests and brought back the results: whether the game closed or hung, what Windows recorded, what WinDbg showed, and eventually what happened with each prototype.

Just saying "it still crashes" wasn't enough to work with. The crash record or debugger output gave us something to compare. Was this the same failure? Had something changed? Did the result support the idea we were testing?

The back-and-forth was roughly:

```text
Try something on my machine
    ↓
Send back what happened, with the relevant output
    ↓
Work out what that result tells us and what to check next
    ↓
Run the next test and see whether the explanation still fits
```

The native debugging part is probably the clearest example. First we had the access violation. Looking into it showed the earlier device failure. That gave us a reason to enable D3D12 validation. The validation error pointed to a call we could break on. Then we could read the descriptor and test a specific change.

I didn't know all those steps when the game first crashed. I was learning what each result meant as we got it, with GPT helping me understand enough to try the next thing.

It also wasn't as organized as that diagram makes it look. We spent time on wrappers that never gave us a working game, and our first serious hook caused another crash. Those are part of the process too. A suggestion could sound reasonable and still fail when I tested it.

For me, the useful part was being able to bring back an unfamiliar result and get help figuring out what to do with it. I still had to run the test. When the result didn't fit, we had to look again. The descriptor correction was only convincing once the device stopped reporting the error and the game actually ran.

## What I'd do again

I don't think the useful lesson here is just "ask GPT to fix it." Most of the useful part was getting a real result, understanding what it actually told us, and then deciding what was worth testing next.

The clean baseline mattered a lot. Once the Vulkan experiments started changing the crash in different ways, going back to native D3D12 gave us something stable to compare against again.

It also helped to keep the observation separate from the explanation. "The splash stays up longer" is a result. "That means we're closer" is a guess. We made a few guesses that went nowhere, so I wouldn't skip that distinction next time.

Same thing with the final fix. Once we had one specific bad call, we tried to change as little as possible. Then when the first hook caused its own crash, we had to treat that as our bug instead of blaming FH3 again.

And I wouldn't stop at a log line saying the hook ran. What mattered was that the device stopped reporting the error and the game actually launched and played.

If I was starting another debugging chat now, I'd probably give the information in a format like this. This isn't copied from the original conversation; it's just how I'd make the same kind of back-and-forth easier to follow next time:

```text
What I'm trying to do:
My hardware and software:
What happens before I change anything:
What I changed for this test:
What happened after that:
The relevant log or debugger output:
What I think might be wrong, and what I'm not sure about:

What can we tell from this, and what still needs checking?
What would you test next, and why?
Please suggest one test and explain what the different results
would tell us. Explain any unfamiliar tools or terms too.
```

That would make it easier to keep track of what we'd tried and why, especially after several tests that didn't work.

## What we actually found

On my test system, FH3 created a null depth-stencil view with an invalid format. D3D12 validation caught it, the device reported `DXGI_ERROR_INVALID_CALL`, and the game then entered its fatal video-card path and crashed. Correcting that exact descriptor stopped the device failure and let the game launch and play.

That doesn't mean every FH3 startup crash has the same cause, or that every Arc GPU behaves the same way. We also didn't prove `D32_FLOAT` was the only valid format we could have used, or establish exactly how every AMD or NVIDIA driver/runtime handles the original call.

Looking back, the fix is small. Getting there wasn't. I started without an error message and tried the usual suggestions on my own. Later, working with GPT, we suspected the driver and spent time trying wrappers. Even after returning to native D3D12, we had to find the earlier device failure before we could inspect the bad call. Then we had to fix our own hook before we could properly test the correction.

I went into this looking for a way around a suspected driver problem. We ended up finding an invalid call from the game and correcting it while keeping native D3D12.

---

## Appendix: Test and implementation details

These are details from the investigation that may help when comparing logs or revisiting the work. They aren't recommended settings or a complete list of every test.

### Translation configuration

The tested combinations included VKD3D-Proton `3.0.1` with DXVK `2.7.1`, and VKD3D `2.8` with DXVK `2.0`. DXVK `2.1` was also suggested, but I don't have a confirmed test result for it. The local DLLs we varied were `d3d12.dll`, `d3d12core.dll`, and `dxgi.dll`.

The configuration and logging variables included:

```text
VKD3D_CONFIG
VKD3D_DISABLE_EXTENSIONS
VKD3D_DEBUG
VKD3D_LOG_FILE
VKD3D_SHADER_CACHE_PATH
VK_DRIVER_FILES
VK_LOADER_LAYERS_DISABLE
DXVK_CONFIG_FILE
DXVK_LOG_LEVEL
DXVK_LOG_PATH
```

Not every variable was present in every run. We removed them when restoring native D3D12. There were package-loading questions in some tests, but the main reason for moving on was that the wrapper experiments hadn't produced a reliable fix or explained the original crash.

### The different failure signatures

These offsets are relative to `forza_x64_release_final.exe`. They came from different test conditions. The game wasn't displaying all of these as error messages.

| Observation | What it came from |
| --- | --- |
| Silent exit, with no displayed error | The initial problem, before we retrieved Windows crash records. |
| `0xc0000005` at `+0x1f28dec` | The native crash recorded by Windows and used as our baseline. Later debugging showed it happened after the graphics-device failure. |
| `+0x21704f2` | A crash during the local-DLL / Vulkan-wrapper tests, not the identified native root cause. |
| `+0x1d006c2` | The changed crash location after disabling `TargetHardwareProfiler.dll`. The game still didn't work. |
| `0x887A0001` / `DXGI_ERROR_INVALID_CALL` | The device-removal reason found before the native fatal crash. |
| `ucrtbase` / `0xc0000005`, and another `0xc0000409` failure | Crashes introduced by the early cloned-vtable shim, separate from the FH3 bug. |

### What the tools told us

`Get-WinEvent` gave us the early Windows crash records for `forza_x64_release_final.exe` and `Microsoft.OpusPG`. Full dumps and WinDbg let us follow the game's fatal path. `GetDeviceRemovedReason` showed the earlier graphics failure.

Windows Graphics Tools and `d3dconfig` let us enable D3D12 diagnostics for the game. The Debug Layer identified the invalid DSV creation. Breaking on Message ID `47` let us inspect the call and descriptor in WinDbg while validation was happening.

DRED was enabled during the device-removal investigation, but it wasn't where we found the bad descriptor. DebugView and `OutputDebugString` helped us watch diagnostic output and check the prototype.

### The later v1.0.1 hardening work

This pass removed mutex/hash-map lookup from the `CreateDepthStencilView` hot path and used a small fixed hook-record table instead. Hook records were published with explicit C++ release/acquire memory ordering. Pointer atomics had to be lock-free, checked at compile time for the supported x64 target.

The implementation also checked the target vtable entry with `VirtualQuery`, documented why `CreateDepthStencilView` is slot `21` on `ID3D12Device`, and failed closed if hook bookkeeping couldn't be set up safely. The exact null-DSV compatibility rule stayed the same.

### The failure and the fix

This is the chain we worked out by the end. We didn't know it when we started:

```text
FH3 creates a null Texture2D depth-stencil view
        ↓
pResource = nullptr, Format = DXGI_FORMAT_UNKNOWN
        ↓
D3D12 validation rejects the format
        ↓
device reports DXGI_ERROR_INVALID_CALL
        ↓
FH3 enters its "Video card" fatal path
        ↓
eventual 0xc0000005 at +0x1f28dec
```

With the working shim:

```text
local d3d12.dll proxy forwards to the system runtime
        ↓
CreateDepthStencilView hook checks the descriptor
        ↓
exact invalid null-DSV pattern?
    ├── no  → forward unchanged
    │
    └── yes → copy descriptor; UNKNOWN → D32_FLOAT
                    ↓
              call the original function
                    ↓
              device remains valid
                    ↓
              FH3 continues startup and runs
```

### Why I wrote this down

The DLL and installation instructions are enough for someone who just wants to play. I also wanted to keep a record of how we found the fix: what we tried, what failed, how we got useful information out of a game that just closed, and where our own code went wrong.

That's the part I'd want to read from someone else's project, especially when learning tools I hadn't used much before.
