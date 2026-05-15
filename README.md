# 1. Склонировать
git clone https://github.com/exhsttd/Desmos2.git

# 2. Собрать через командную строку
cd Desmos2/Desmos2

# 3. Собрать
mkdir build
cd build
cmake .. -DDOXYGEN_FOUND:BOOL=OFF
cmake --build . --config Release

# 4. Запустить
.\Release\Desmos2.exe

# 5. Радоваться
