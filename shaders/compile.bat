@echo off
cls

set "__glslc=C:\Dev\_libraries\Vulkan\Bin\glslc.exe"

setlocal ENABLEDELAYEDEXPANSION

rem Create the compiled directory if it doesn't exist
if not exist compiled mkdir compiled

@rem %__glslc% shader.vert -o vert_%vertexShaderCount%.spv
for %%f in (*.vert, *.frag) do (
    set "precompiled_file=%%f"
    set "filename=%%~nf"
    set "extension=%%~xf"
    rem Remove the "." from the extension
    set "compiled_file=compiled\!filename!_!extension:~1!.spv"

    !__glslc! !precompiled_file! -o !compiled_file!

    echo !compiled_file! compiled
)

endlocal

set __glslc=