cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

file(
    GLOB SOURCE_LIST CONFIGURE_DEPENDS
    "${SpotGridBot_SOURCE_DIR}/src/*.cpp"
    "${SpotGridBot_SOURCE_DIR}/src/binance/*.cpp"
    "${SpotGridBot_SOURCE_DIR}/src/coinbase/*.cpp"
    "${SpotGridBot_SOURCE_DIR}/src/OKX/*.cpp"
    )

add_executable(SpotGridBot ${SOURCE_LIST})

target_include_directories(SpotGridBot PRIVATE
        "${SpotGridBot_SOURCE_DIR}/include"
        "${SpotGridBot_SOURCE_DIR}/lib/util/include"
        )

target_link_libraries(SpotGridBot PUBLIC
                        PocoFoundation
                        PocoUtil
                        PocoNet
                        PocoNetSSL
                        PocoJSON
                        PocoData
                        protobuf
                        crypto
                        ${Utils_BINARY_DIR}/libUtils.so
                        )

install(TARGETS SpotGridBot DESTINATION bin)