// Copyright 2024 The TrueBlocks Authors. All rights reserved.
// Use of this source code is governed by a license that can
// be found in the LICENSE file.
/*
 * Parts of this file were generated with makeClass --run. Edit only those parts of
 * the code inside of 'EXISTING_CODE' tags.
 */

package sdk

import (
	"io"

	daemon "github.com/TrueBlocks/trueblocks-core/src/apps/chifra/sdk"
)

// Daemon does chifra daemon
func Daemon(w io.Writer, options map[string]string) error {
	return daemon.Daemon(w, options)
}

// EXISTING_CODE
// EXISTING_CODE

