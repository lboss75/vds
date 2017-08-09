echo Create root
set root_folder=%~d0%~p0
rmdir %root_folder%servers\1 /s /q

echo creating server
build\app\vds_background\Debug\vds_background.exe server root -p 123qwe -r %root_folder%servers\1

echo starting server
start build\app\vds_background\Debug\vds_background.exe server start -r %root_folder%servers\1

echo upload file
rem build\app\vds_node\Debug\vds_node.exe file upload -l root -p 123qwe -f %root_folder%build\app\vds_node\Debug\vds_node.exe -r %root_folder%clients\1

echo download file
rem build\app\vds_node\Debug\vds_node.exe file download -l root -p 123qwe -f %root_folder%build\vds_node.exe -r %root_folder%clients\1