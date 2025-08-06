project(OrderBook
        LANGUAGES CXX C
        DESCRIPTION "OrderBook Lib used by SpotGridBot"
        )

set(SOURCE_FILES
        ${OrderBook_SOURCE_DIR}/src/Quote.cpp
        ${OrderBook_SOURCE_DIR}/src/OrderBook.cpp
        ${OrderBook_SOURCE_DIR}/src/BookView.cpp
)

add_library(OrderBook SHARED ${SOURCE_FILES})

target_include_directories(OrderBook PUBLIC ${OrderBook_SOURCE_DIR}/include)

#add_subdirectory(tests)
