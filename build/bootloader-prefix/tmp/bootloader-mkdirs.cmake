# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Esso/esp/v5.1.4/esp-idf/components/bootloader/subproject"
  "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader"
  "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader-prefix"
  "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader-prefix/tmp"
  "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader-prefix/src/bootloader-stamp"
  "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader-prefix/src"
  "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "H:/OneDrive/Documents/esp-idf/Projects/DogbulEsp32/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
