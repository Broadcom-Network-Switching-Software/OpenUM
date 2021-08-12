/*! \file shr_debug.h
 *
 * Generic macros for tracing function call trees.
 *
 * The main principle is to establish a single point of exit for each
 * function, and then combine this with a standard method of logging
 * error conditions.
 */
/*
 * This license is set out in https://raw.githubusercontent.com/Broadcom-Network-Switching-Software/OpenUM/master/Legal/LICENSE file.
 * 
 * Copyright 2007-2021 Broadcom Inc. All rights reserved.
 */

#ifndef SHR_DEBUG_H
#define SHR_DEBUG_H

#define CFG_SHR_DEBUG_LEVEL_ERROR    0
#define CFG_SHR_DEBUG_LEVEL_DEBUG    1
#define CFG_SHR_DEBUG_LEVEL_TRACE    2

#define SHR_NO_UNIT              -1

/* Print error message only default. */
#ifndef CFG_SHR_DEBUG_LEVEL
#define CFG_SHR_DEBUG_LEVEL     CFG_SHR_DEBUG_ERROR
#endif

/*! Allow function enter/exit logging to be compiled out. */
#if defined(CFG_SHR_DEBUG_INCLUDED) && defined(CFG_DEBUGGING_INCLUDED)

/* Enable trace message. */
#if CFG_SHR_DEBUG_LEVEL >= CFG_SHR_DEBUG_LEVEL_TRACE
#define SHR_LOG_TRACE(fmt, ...)                             \
    do {                                                     \
        sal_printf("%s:%d Trace: ", __FILENAME__, __LINE__); \
        sal_printf(fmt, ##__VA_ARGS__);                     \
    } while(0)
#endif

/* Enable debug message. */
#if CFG_SHR_DEBUG_LEVEL >= CFG_SHR_DEBUG_LEVEL_DEBUG
#define SHR_LOG_DEBUG(fmt, ...)                             \
    do {                                                     \
        sal_printf("%s:%d Debug: ", __FILENAME__, __LINE__); \
        sal_printf(fmt, ##__VA_ARGS__);                     \
    } while(0)
#endif

/* Enable error messsage log. */
#if CFG_SHR_DEBUG_LEVEL >= CFG_SHR_DEBUG_LEVEL_ERROR
#define SHR_LOG_ERROR(fmt, ...)                             \
    do {                                                     \
        sal_printf("%s:%d Error: ", __FILENAME__, __LINE__); \
        sal_printf(fmt, ##__VA_ARGS__);                     \
    } while(0)
#endif

#endif

#ifndef SHR_LOG_ERROR
#define SHR_LOG_ERROR(args, ...) do { } while(0)
#endif

#ifndef SHR_LOG_DEBUG
#define SHR_LOG_DEBUG(args, ...) do { } while(0)
#endif

#ifndef SHR_LOG_TRACE
#define SHR_LOG_TRACE(args, ...) do { } while(0)
#endif

#define SHR_FAILURE(rv)      (rv != SYS_OK)
#define SHR_SUCCESS(rv)      (rv == SYS_OK)

/*!
 * \brief Function entry declarations and code.
 *
 * This macro must appear in each function right after the local
 * variable declarations.
 *
 * The macro declares a temporary return value variable for use by the
 * error checking macros, because if this variable is declared locally
 * within each macro, the stack usage increases significantly if error
 * checking macros are used extensively in a single function.
 *
 * The associated debug log message will use BSL_LOG_MODULE as the log
 * source, so this name must be assigned to a relevant BSL log source
 * in advance.
 *
 * Example:
 *
 * \code{.c}
 * void my_func(int unit)
 * {
 *     int local_idx;
 *
 *     SHR_FUNC_ENTER(unit);
 *     ...
 * }
 * \endcode
 *
 * \param [in] _unit Switch unit number.
 */
#define SHR_FUNC_ENTER(_unit)                           \
    int _func_unit = _unit;                             \
    sys_error_t _func_rv = SYS_OK;                      \
    sys_error_t _tmp_rv;                                \
    (void)_tmp_rv;                                      \
    (void)_func_unit;                                   \
    (void)_func_rv;                                     \
    SHR_LOG_TRACE("%s enter\n", __FUNCTION__)

/*!
 * \brief Single point of exit code.
 *
 * This macro must appear at the very bottom of each function, and it
 * must be preceded an 'exit' label and optionally some resource
 * clean-up code.
 *
 * The associated debug log message will use BSL_LOG_MODULE as the log
 * source, so this name must be assigned to a relevant BSL log source
 * in advance.
 *
 * Example:
 *
 * \code{.c}
 * void my_func(int unit)
 * {
 *     int local_idx;
 *
 *     SHR_FUNC_ENTER(unit);
 *     ...
 *
 *   exit:
 *     SHR_FUNC_EXIT();
 * }
 * \endcode
 */
#define SHR_FUNC_EXIT()                                \
    SHR_LOG_TRACE("%s exit\n", __FUNCTION__);          \
    return _func_rv;

/*!
 * \brief Goto single point of exit.
 *
 * Go to label 'exit', which must be defined explicitly in each
 * function.
 *
 * This macro is normally not called directly, but it is used as part
 * of other error handling macros.
 */
#define SHR_EXIT() goto exit

/*!
 * \brief Error-exit.
 *
 * Log an error message and go to the function's single point of exit.
 *
 * Example:
 *
 * \code{.c}
 * void my_func(int unit)
 * {
 *     int local_idx;
 *
 *     SHR_FUNC_ENTER(unit);
 *
 *     if (drv_lookup(unit, local_idx) == NULL) {
 *         SHR_ERR_EXIT(SHR_E_NOT_FOUND);
 *     }
 *     ...
 *
 *   exit:
 *     SHR_FUNC_EXIT();
 * }
 * \endcode
 *
 * \param [in] _rv Error code.
 */
#define SHR_ERR_EXIT(_rv)                                         \
    do {                                                          \
        _func_rv = _rv;                                           \
        if (SHR_FAILURE(_rv)) {                                    \
            SHR_LOG_ERROR("Exit with error code %d\n", _func_rv); \
        }                                                         \
        SHR_EXIT();                                               \
    } while (0)

/*!
 * \brief Error-exit on expression error.
 *
 * Evaluate an expression and if it evaluates to a standard error
 * code, then log an error message and go to the function's single
 * point of exit.
 *
 * Example:
 *
 * \code{.c}
 * void my_func(int unit)
 * {
 *     int local_idx;
 *
 *     SHR_FUNC_ENTER(unit);
 *
 *     SHR_IF_ERR_EXIT
 *         (some_other_func(unit));
 *     ...
 *
 *   exit:
 *     SHR_FUNC_EXIT();
 * }
 * \endcode
 *
 * \param [in] _expr Expression to evaluate.
 */
#define SHR_IF_ERR_EXIT(_expr)                                   \
    do {                                                         \
        _tmp_rv = _expr;                                         \
        if (SHR_FAILURE(_tmp_rv)) {                              \
            SHR_ERR_EXIT(_tmp_rv);                               \
        }                                                        \
    } while (0)

/*!
 * \brief Warn on expression error.
 *
 * Evaluate an expression and log a warning message if the expression
 * evaluates to a standard error code. The result is assigned to the
 * macro-based function return value (declared by \ref
 * SHR_FUNC_ENTER), but code execution continues immediately below the
 * statement (i.e. no jump to the single point of exit).
 *
 * Example:
 *
 * \code{.c}
 * void my_func(int unit)
 * {
 *     int local_idx;
 *
 *     SHR_FUNC_ENTER(unit);
 *
 *     SHR_IF_ERR_CONT(some_other_func(unit));
 *     ...
 *
 *   exit:
 *     SHR_FUNC_EXIT();
 * }
 * \endcode
 *
 * \param [in] _expr Expression to evaluate.
 */
#define SHR_IF_ERR_CONT(_expr)                  \
    do {                                        \
        _tmp_rv = _expr;                        \
        if (SHR_FAILURE(_tmp_rv)) {             \
            _func_rv = _tmp_rv;                 \
        }                                       \
    } while (0)

/*!
 * \brief Check for function error state.
 *
 * This macro is a Boolean expression, which evaluates to TRUE, if the
 * macro-based function return value (declared by \ref SHR_FUNC_ENTER)
 * is set to error.
 *
 * It can be used to clean up allocated resources in case of failure,
 * for example:
 *
 * \code{.c}
 * void my_func(int unit)
 * {
 *     int local_idx;
 *
 *     SHR_FUNC_ENTER(unit);
 *     ...
 *
 *   exit:
 *     if (SHR_FUNC_ERR()) {
 *         ...
 *     }
 *     SHR_FUNC_EXIT();
 * }
 * \endcode
 */
#define SHR_FUNC_ERR()                          \
    SHR_FAILURE(_func_rv)

/*!
 * \brief Check for null-pointer.
 *
 * Check if a pointer is NULL, and if so, log an error and exit.
 *
 * The macro is intended for both input parameter checks and memory
 * allocation errors.
 *
 * \param [in] _ptr Pointer to check.
 * \param [in] _rv Function return value to use if pointer is NULL.
 */
#define SHR_NULL_CHECK(_ptr, _rv)                       \
    do {                                                \
        if ((_ptr) == NULL) {                           \
            SHR_LOG_ERROR("Null pointer\n");            \
            SHR_IF_ERR_EXIT(_rv);                       \
        }                                               \
    } while (0)

#endif /* SHR_DEBUG_H */
