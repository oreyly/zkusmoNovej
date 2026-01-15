# CMakeList.txt : CMake project for zkusmoNovej, include source and define
# project specific logic here.
#

# Add source to this project's executable.
add_executable (zkusmoNovej "ExternalLibraries/magic_enum.hpp" "Sources/program.cpp" "Headers/program.h" "Sources/ComunicatorO.cpp"  "Headers/Errors.h" "Sources/Errors.cpp" "Sources/Logger.cpp" "Headers/Logger.h" "Sources/starejkod.cpp" "Sources/Client.cpp" "Headers/Client.h" "Headers/Comunicator.h" "Sources/Comunicator.cpp" "Sources/RequestManager.cpp" "Headers/Message.h" "Headers/Packet.h" "Sources/Packet.cpp" "Headers/Utils.h" "Sources/Utils.cpp" "Sources/Message.cpp" "Headers/RequestManager.h" "Sources/RequestManager.cpp" "Headers/Request.h" "Sources/Request.cpp" "Headers/MessageManager.h" "Sources/MessageManager.cpp"  "Headers/Chessboard.h" "Sources/Chessboard.cpp" "Headers/opcode.h" "Headers/GameLobby.h" "Headers/GameRoom.h" "Sources/GameLobby.cpp" "Sources/GameRoom.cpp")

# Přidání složky s hlavičkovými soubory do vyhledávací cesty
target_include_directories(zkusmoNovej PRIVATE Headers)
target_include_directories(zkusmoNovej PRIVATE ExternalLibraries)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET zkusmoNovej PROPERTY CXX_STANDARD 20)
endif()

# Provide definitions to help Visual Studio IntelliSense
if(CMAKE_VS_INTEL_SENSE_VERSION)
    add_compile_definitions(_Float32=float)
endif()

# TODO: Add tests and install targets if needed.
