set(app_src acs_pinout.c)

idf_component_register(SRCS ${app_src}
                    REQUIRES "driver"
                    INCLUDE_DIRS "."
                    )