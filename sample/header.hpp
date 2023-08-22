#pragma once

/**
 * @brief Allows subscribe
 * @param jobId The ID of the job to subscribe to its events.
 * @param callbackIndex Index of the callback to be executed to forward events.
 * @param userData User parameter passed as-is to the callback.
 */
extern void subscribeEvents(const int jobId, void (*callbackIndex)(int userData1), const unsigned int userData);
