idf_component_register(SRCS "./init/init.c"
                            "./event/event.c"
                            "./event/debug.c"
                            "./storage/storage.c"
                            "./payload/payload.c"
                            "./handlers/wakeup.c"
                            "./handlers/handlers.c"
                            "./handlers/debug.c"
                            "./handlers/backoff.c"
                            "main.c"
                       INCLUDE_DIRS "include")
