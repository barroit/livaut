set(SCHEDULE_INPUT "${CMAKE_SOURCE_DIR}/schedule.in")
set(SCHEDULE_DEF "signal-schedule-def.h")
set(SCHEDULE_LIST "signal-schedule.h")
set(SCHEDULE_EXEC "${CMAKE_SOURCE_DIR}/make-schedule")

add_custom_command(OUTPUT ${SCHEDULE_DEF} ${SCHEDULE_LIST}
		   COMMAND ${SCHEDULE_EXEC} ${SCHEDULE_INPUT}
		   ${SCHEDULE_DEF} ${SCHEDULE_LIST}
		   DEPENDS ${SCHEDULE_INPUT})

add_custom_target(schedule DEPENDS ${SCHEDULE_DEF} ${SCHEDULE_LIST})
add_dependencies(${COMPONENT_LIB} schedule)

set_property(DIRECTORY ${COMPONENT_DIR} APPEND PROPERTY
	     ADDITIONAL_CLEAN_FILES ${SCHEDULE_OUTPUT})
