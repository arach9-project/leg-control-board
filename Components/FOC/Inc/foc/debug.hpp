

#ifndef __SIMPLEFOCDEBUG_H__
#define __SIMPLEFOCDEBUG_H__

#ifndef SIMPLEFOC_DISABLE_DEBUG
#define SIMPLEFOC_MOTOR_WARN(msg, ...)                                         \
    SimpleFOCDebug::print(MOT_WARN);                                           \
    SIMPLEFOC_DEBUG(msg, ##__VA_ARGS__)

#define SIMPLEFOC_MOTOR_ERROR(msg, ...)                                        \
    SimpleFOCDebug::print(MOT_ERR);                                            \
    SIMPLEFOC_DEBUG(msg, ##__VA_ARGS__)

#define SIMPLEFOC_MOTOR_DEBUG(msg, ...)                                        \
    SimpleFOCDebug::print(MOT_DEBUG);                                          \
    SIMPLEFOC_DEBUG(msg, ##__VA_ARGS__)

#else
#define SIMPLEFOC_MOTOR_DEBUG(msg, ...)
#define SIMPLEFOC_MOTOR_ERROR(msg, ...)
#define SIMPLEFOC_MOTOR_WARN(msg, ...)
#endif
#endif
