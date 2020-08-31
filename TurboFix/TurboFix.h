#pragma once

#ifdef TF_EXPORTS
#define TF_API extern "C" __declspec(dllexport)
#else
#define TF_API extern "C" __declspec(dllimport)
#endif

/**
 * \brief           Get whether the TurboFix is active or not.
 *                  When active, turbo behavior for the current player vehicle
 *                  is modified.
 * \return          True: TurboFix is active. False: TurboFix is not active.
 */
TF_API bool TF_Active();

/**
 * \brief           Get normalized turbo boost.
 * \return          Boost level mapped from min/max to -1.0 to 1.0. 0 if not active.
 */
TF_API float TF_GetNormalizedBoost();

/**
 * \brief           Get absolute turbo boost.
 * \return          Boost level without mapping/processing. 0 if not active.
 */
TF_API float TF_GetAbsoluteBoost();

/**
 * \brief           Get min turbo boost.
 * \return          Min turbo boost level. 0 if not active.
 */
TF_API float TF_GetAbsoluteBoostMin();


/**
 * \brief           Get max turbo boost.
 * \return          Max turbo boost level. 0 if not active.
 */
TF_API float TF_GetAbsoluteBoostMax();
