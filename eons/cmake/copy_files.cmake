# Assets
if(EXISTS "${SOURCE_DIR}/eons/assets")
    file(COPY "${SOURCE_DIR}/eons/assets/" DESTINATION "${TARGET_DIR}/assets")
endif()

# Config files
file(COPY "${SOURCE_DIR}/eons/game.ini" DESTINATION "${TARGET_DIR}")
file(COPY "${SOURCE_DIR}/eons/engine.ini" DESTINATION "${TARGET_DIR}")
