# naiad ocean toolkit

remove-item $env:DEV_ROOT_NBUDDY_RENDERMAN/stage-$env:EM_PLAT-$env:EM_ARCH -force -recurse
new-item $env:DEV_ROOT_NBUDDY_RENDERMAN/stage-$env:EM_PLAT-$env:EM_ARCH -force -type directory 

new-item $env:DEV_ROOT_NBUDDY_RENDERMAN/stage-$env:EM_PLAT-$env:EM_ARCH/debug -force -type directory
new-item $env:DEV_ROOT_NBUDDY_RENDERMAN/stage-$env:EM_PLAT-$env:EM_ARCH/release -force -type directory

$env:dcmakecmd = "cmake -DCMAKE_BUILD_TYPE=DEBUG -DCMAKE_INSTALL_PREFIX=$env:NAIAD_PATH $env:DEV_ROOT_NBUDDY_RENDERMAN -G `"Visual Studio 10 Win64`""
$env:rcmakecmd = "cmake -DCMAKE_BUILD_TYPE=RELEASE -DCMAKE_INSTALL_PREFIX=$env:NAIAD_PATH $env:DEV_ROOT_NBUDDY_RENDERMAN -G `"Visual Studio 10 Win64`""

cd $env:DEV_ROOT_NBUDDY_RENDERMAN/stage-$env:EM_PLAT-$env:EM_ARCH/debug
$env:dcmakecmd
Invoke-Expression -Command $env:dcmakecmd

cd $env:DEV_ROOT_NBUDDY_RENDERMAN/stage-$env:EM_PLAT-$env:EM_ARCH/release
$env:rcmakecmd
Invoke-Expression -Command $env:rcmakecmd

cd ~
