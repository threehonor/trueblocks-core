// Copyright 2021 The TrueBlocks Authors. All rights reserved.
// Use of this source code is governed by a license that can
// be found in the LICENSE file.

package whenPkg

import (
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/identifiers"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/logger"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/tslib"
)

// HandleTimestampsRepair handles chifra when --timestamps --reset <bn> to reset a single block's timestamps (call repeatedly if needed)
func (opts *WhenOptions) HandleTimestampsRepair() error {
	cnt, err := tslib.NTimestamps(opts.Globals.Chain)
	if err != nil {
		return err
	}

	// skip := uint64(500)
	// scanBar := progress.NewScanBar(cnt /* wanted */, (cnt / skip) /* freq */, cnt /* max */, (2. / 3.))

	blockNums, err := identifiers.GetBlockNumbers(opts.Globals.Chain, opts.BlockIds)
	if err != nil {
		return err
	}

	for _, bn := range blockNums {
		if bn < cnt { // ranges may include blocks after last block
			if err := tslib.Repair(opts.Globals.Chain, bn); err != nil {
				return err
			}
			if bn == 1 { // weird special case because because I don't know how to get Cobra to handle non-zero defaults
				if err := tslib.Repair(opts.Globals.Chain, 0); err != nil {
					return err
				}
			}

			ts, _ := tslib.FromBnToTs(opts.Globals.Chain, bn)
			logger.Log(logger.Info, "The timestamp at block", bn, "was reset to", ts, "from on chain.")
		}
	}

	return nil
}