set(app_src battery_rs485.c)

idf_component_register(SRCS ${app_src}
                    REQUIRES "driver"
                    INCLUDE_DIRS "."
                    )