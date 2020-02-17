
########### sqlite ###############

set( amarok_collection-sqlitecollection_PART_SRCS
    SQLiteCollectionFactory.cpp
    SQLiteQueryMaker.cpp
)

add_library(amarok_collection-sqlitecollection MODULE ${amarok_collection-sqlitecollection_PART_SRCS})

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

target_link_libraries(amarok_collection-sqlitecollection
    amarok-sqlcollection
    amarokcore
    amaroklib
    sqlite_cpp
    sqlite3
    Qt5::Core
    Qt5::Gui
    ${CMAKE_DL_LIBS}
    ${ZLIB_LIBRARIES}
)

if(APPLE)
	SET_TARGET_PROPERTIES(amarok_collection-sqlitecollection PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

install(TARGETS amarok_collection-sqlitecollection DESTINATION ${KDE_INSTALL_PLUGINDIR} )

install(FILES amarok_collection-sqlitecollection.desktop DESTINATION ${KDE_INSTALL_KSERVICES5DIR})

kcoreaddons_desktop_to_json(amarok_collection-sqlitecollection amarok_collection-sqlitecollection.desktop SERVICE_TYPES ${CMAKE_SOURCE_DIR}/src/amarok-plugin.desktop)
