echo Create root
set root_folder=%~d0%~p0

rmdir %root_folder%servers /s /q
rmdir %root_folder%clients /s /q
del /q %root_folder%vds.log.new

echo creating server
build\app\vds_background\Debug\vds_background.exe server root -p 123qwe -r %root_folder%servers\1

echo starting server
start build\app\vds_background\Debug\vds_background.exe server start -r %root_folder%servers\1

build\app\vds_node\Debug\vds_node.exe node install -l root -p 123qwe -r D:\projects\vds\servers\2

echo upload file
build\app\vds_node\Debug\vds_node.exe file upload -l root -p 123qwe -f %root_folder%vds.log -r %root_folder%clients\2

echo download file
build\app\vds_node\Debug\vds_node.exe file download -l root -p 123qwe -f %root_folder%vds.log.new -n vds.log -r %root_folder%clients\1

fc /b %root_folder%vds.log %root_folder%vds.log.new