# assets directory
if(EXISTS "${SOURCE_DIR}/assets")
    file(COPY "${SOURCE_DIR}/assets/" DESTINATION "${TARGET_DIR}/assets")
endif()

set(INI_SRC "${SOURCE_DIR}/src/engine/engine.ini")
set(INI_DEST "${TARGET_DIR}/engine.ini")
set(GAME_INI_SRC "${SOURCE_DIR}/src/game/game.ini")
set(GAME_INI_DEST "${TARGET_DIR}/game.ini")

file(COPY "${INI_SRC}" DESTINATION "${TARGET_DIR}")
file(COPY "${GAME_INI_SRC}" DESTINATION "${TARGET_DIR}")
