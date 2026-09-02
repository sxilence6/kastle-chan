#
# MIT License
# Copyright (c) 2025 Vaclav Mach (Bastl Instruments)
#

# App definition with included samples
create_kastle2_app(
    APP_NAME "kastle-chan"
    APP_NAME_WITH_USER_DATA "kastle-chan-with-voice"
    APP_USB_NAME "Kastle-chan"
    APP_USB_PREFIX "K2KC_"
    APP_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/AppKastleChan.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/GrammarEngine.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ToyPianoEngine.cpp
    APP_USER_DATA ${CMAKE_CURRENT_SOURCE_DIR}/SAMPLES.bin
)
