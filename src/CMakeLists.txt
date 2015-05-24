# Plik konfiguracyjny CMake do zadania zaliczeniowego.
# Autor: Tomasz Kociumaka

# zabezpieczamy się przed niekompatybilnością ze starszymi wersjami CMake
cmake_minimum_required (VERSION 2.6)

# nazwa projektu i język źródeł
project (Spellcheck C)    

# deklarujemy opcję DEBUG, domyślnie wyłączoną (opcję można włączyć przez argument -DDEBUG=ON)
option (DEBUG OFF)

# jeśli DEBUG == ON, ustawiamy zmienną specyfikującą typ kompilacji na wartość DEBUG, wpp. na release
if (DEBUG)
    set(CMAKE_BUILD_TYPE DEBUG)
else (DEBUG)
    set(CMAKE_BUILD_TYPE RELEASE)
endif (DEBUG)

# ustawiamy flagi kompilacji w wersji debug i release
set(CMAKE_C_FLAGS_DEBUG "-std=gnu99 -Wall -pedantic -g")
set(CMAKE_C_FLAGS_RELEASE "-std=gnu99 -O3")

# wskazujemy na foldery, gdzie znajdują się pliki nagłówkowe.
include_directories (dictionary)

# wskazujemy na foldery, gdzie znajdują się szczegółowe pliki CMakeLists.txt
add_subdirectory (dictionary)
add_subdirectory (dict-editor)
add_subdirectory (dict-check)


# dodajemy obsługę Doxygena: sprawdzamy, czy jest zainstalowany i jeśli tak:
find_package(Doxygen)
if(DOXYGEN_FOUND)
    # wskazujemy lokalizacją pliku konfiguracyjnego i podajemy jego docelową lokalizację w folderze, gdzie następuje kompilacja
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
    # dodajemy cel doc: użycie make doc spowoduje wywołanie doxygena z odpowiednim plikiem konfiguracyjnym w folderze kompilacji. 
    # Na wyjście zostanie wypisana odpowiedni komentarz.
    add_custom_target(doc
            ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMENT "Generating API documentation with Doxygen"
            )
endif(DOXYGEN_FOUND)
