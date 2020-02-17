########### sqlite ###############

include_directories(${SQLITE_INCLUDE_DIR})

set( amarok_storage-sqlitestorage_PART_SRCS
     SQLiteStorage.cpp
     SQLiteStorageFactory.cpp
)

add_library(amarok_storage-sqlitestorage MODULE ${amarok_storage-sqlitestorage_PART_SRCS})

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_STATIC_LINKER_FLAGS_NOFATALWARN "${CMAKE_STATIC_LINKER_FLAGS}")
set(CMAKE_STATIC_LINKER_FLAGS ${CMAKE_STATIC_LINKER_FLAGS_NOFATALWARN} )

string(REPLACE "-Wl,--fatal-warnings" "" CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN "${CMAKE_MODULE_LINKER_FLAGS}")
set(CMAKE_MODULE_LINKER_FLAGS ${CMAKE_MODULE_LINKER_FLAGS_NOFATALWARN} )

find_library(sqlite_cpp libSQLiteCpp.a)
add_library(sqlite_cpp STATIC IMPORTED)
set_target_properties(sqlite_cpp PROPERTIES IMPORTED_LOCATION ${sqlite_cpp})

find_library(sqlite3 libsqlite3.a)
add_library(sqlite3 STATIC IMPORTED)
set_target_properties(sqlite3 PROPERTIES IMPORTED_LOCATION ${sqlite3})
target_link_libraries(sqlite3 INTERFACE pthread dl)


target_link_libraries(amarok_storage-sqlitestorage
    sqlite_cpp
    sqlite3
    amarokcore
    amaroklib
    Qt5::Core
    Qt5::Gui
    ${SQLITE_LIBRARIES}
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(${CMAKE_HOST_SYSTEM_NAME} MATCHES "FreeBSD")
        target_link_libraries(amarok_storage-sqlitestorage ${SQLITE_LIBRARIES})
endif()

if(NOT WIN32 AND NOT APPLE)
    target_link_libraries( amarok_storage-sqlitestorage crypt pthread )
endif()

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_storage-sqlitestorage PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_storage-sqlitestorage DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES amarok_storage-sqlitestorage.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

kcoreaddons_desktop_to_json(amarok_storage-sqlitestorage amarok_storage-sqlitestorage.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)
