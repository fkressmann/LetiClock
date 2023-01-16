import re
import shutil
import glob
import os

version_cpp_pattern = re.compile('#define VERSION *"(.+)"')
cpp_file = open('./../src/main.cpp', 'r')
version = version_cpp_pattern.findall(cpp_file.read())[0]
cpp_file.close()

server_version_pattern = re.compile('(?<="80:7D:3A:79:01:7B" => ")(.*)(?=")')
server_file = open("server.php", 'r')
server_file_content = server_version_pattern.sub(version, server_file.read())
server_file.close()
server_file = open("server.php", 'w')
server_file.write(server_file_content)
server_file.close()

[os.remove(x) for x in glob.glob('*.bin')]
shutil.copy2('./../.pio/build/production/firmware.bin', f"./{version}.bin")