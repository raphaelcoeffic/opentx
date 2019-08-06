include_directories(
  ${RADIO_SRC_DIRECTORY}/gui/colorlcd
  ${RADIO_SRC_DIRECTORY}/gui/colorlcd/layouts
  ${RADIO_SRC_DIRECTORY}/fonts/truetype
)

file(GLOB THEMES_SRC RELATIVE ${RADIO_SRC_DIRECTORY}/gui/colorlcd ${RADIO_SRC_DIRECTORY}/gui/colorlcd/themes/*.cpp)
file(GLOB LAYOUTS_SRC RELATIVE ${RADIO_SRC_DIRECTORY}/gui/colorlcd ${RADIO_SRC_DIRECTORY}/gui/colorlcd/layouts/*.cpp)
file(GLOB WIDGETS_SRC RELATIVE ${RADIO_SRC_DIRECTORY}/gui/colorlcd ${RADIO_SRC_DIRECTORY}/gui/colorlcd/widgets/*.cpp)

set(GUI_SRC
  ${GUI_SRC}
  lcd.cpp
  splash.cpp
  fonts.cpp
  curves.cpp
  bitmaps.cpp

  theme.cpp
  #  topbar.cpp
  #  layout.cpp
  #  widget.cpp
  ${THEMES_SRC}
  #  ${LAYOUTS_SRC}
  #  ${WIDGETS_SRC}
  rle.cpp
  #  popups.cpp
  draw_functions.cpp
  #  menus.cpp
  menu_model.cpp
  #  model_select.cpp
  model_setup.cpp
  model_flightmodes.cpp
  model_inputs.cpp
  model_mixes.cpp
  model_outputs.cpp
  model_curves.cpp
  model_gvars.cpp
  model_logical_switches.cpp
  special_functions.cpp
  model_telemetry.cpp
  menu_radio.cpp
  radio_setup.cpp
  radio_sdmanager.cpp
  radio_tools.cpp
  radio_trainer.cpp
  radio_version.cpp
  radio_hardware.cpp
  radio_calibration.cpp
  view_main.cpp
  #  view_statistics.cpp
  )

macro(add_gui_src src)
  set(GUI_SRC
    ${GUI_SRC}
    ${src}
    )
endmacro(add_gui_src)

if(HELI)
  add_gui_src(model_heli.cpp)
endif()

if(PXX2 OR LUA)
  add_gui_src(radio_tools.cpp)
endif()

if(PXX2)
  add_gui_src(radio_spectrum_analyser.cpp)
endif()

if(LUA STREQUAL YES)
  # TODO set(SRC ${SRC} lua/widgets.cpp)
  # TODO set(GUI_SRC ${GUI_SRC} model_custom_scripts.cpp)
endif()

# gui/${GUI_DIR}/modelslist.cpp

set(SRC
  ${SRC}
  stubs.cpp
  )

# includes libopenui
include(gui/libopenui/CMakeLists.txt)
include_directories(gui/libopenui)
add_definitions(-DLIBOPENUI)
foreach(LIBOPENUI_FILE ${LIBOPENUI_SRC})
  set(SRC ${SRC} gui/libopenui/${LIBOPENUI_FILE})
endforeach()