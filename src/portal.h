/*
 * SPDX-License-Identifier: GPL-3.0
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 */

#pragma once
#include "config_manager.h"

// Start a WiFi Access Point and serve a configuration web page.
// Blocks until the user submits the form; saves config and reboots.
// apName must be a null-terminated string (e.g. "ePaperFrame-A1B2").
void runConfigPortal(Config& cfg, const char* apName);
