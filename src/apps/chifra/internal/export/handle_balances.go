// Copyright 2021 The TrueBlocks Authors. All rights reserved.
// Use of this source code is governed by a license that can
// be found in the LICENSE file.

package exportPkg

import (
	"context"
	"fmt"
	"math/big"
	"sort"

	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/base"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/filter"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/logger"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/monitor"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/names"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/output"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/tslib"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/types"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/utils"
)

func (opts *ExportOptions) HandleBalances(monitorArray []monitor.Monitor) error {
	chain := opts.Globals.Chain
	testMode := opts.Globals.TestMode
	nErrors := 0

	filter := filter.NewFilter(
		opts.Reversed,
		opts.Reverted,
		opts.Fourbytes,
		base.BlockRange{First: opts.FirstBlock, Last: opts.LastBlock},
		base.RecordRange{First: opts.FirstRecord, Last: opts.GetMax()},
	)

	ctx, cancel := context.WithCancel(context.Background())
	fetchData := func(modelChan chan types.Modeler[types.RawToken], errorChan chan error) {
		currentBn := uint64(0)
		prevBalance := big.NewInt(0)
		visitToken := func(idx int, item *types.SimpleToken) error {
			item.PriorBalance = *prevBalance
			if item.BlockNumber == 0 || item.BlockNumber != currentBn {
				item.Timestamp, _ = tslib.FromBnToTs(chain, item.BlockNumber)
			}
			currentBn = item.BlockNumber
			if idx == 0 || item.PriorBalance.Cmp(&item.Balance) != 0 || opts.Globals.Verbose {
				item.Diff = *big.NewInt(0).Sub(&item.Balance, &item.PriorBalance)
				modelChan <- item
			}
			prevBalance = &item.Balance
			return nil
		}

		for _, mon := range monitorArray {
			if sliceOfMaps, cnt, err := monitor.AsSliceOfMaps[types.SimpleToken](&mon, filter); err != nil {
				errorChan <- err
				cancel()

			} else if cnt == 0 {
				errorChan <- fmt.Errorf("no appearances found for %s", mon.Address.Hex())
				continue

			} else {
				bar := logger.NewBar(logger.BarOptions{
					Prefix:  mon.Address.Hex(),
					Enabled: !testMode && !utils.IsTerminal(),
					Total:   int64(cnt),
				})

				for _, thisMap := range sliceOfMaps {
					thisMap := thisMap
					for app := range thisMap {
						thisMap[app] = new(types.SimpleToken)
					}

					iterFunc := func(app types.SimpleAppearance, value *types.SimpleToken) error {
						var balance *big.Int
						if balance, err = opts.Conn.GetBalanceByAppearance(mon.Address, &app); err != nil {
							return err
						}
						value.Address = base.FAKE_ETH_ADDRESS
						value.Holder = mon.Address
						value.BlockNumber = uint64(app.BlockNumber)
						value.TransactionIndex = uint64(app.TransactionIndex)
						value.Balance = *balance
						value.Timestamp = app.Timestamp
						bar.Tick()
						return nil
					}

					iterErrorChan := make(chan error)
					iterCtx, iterCancel := context.WithCancel(context.Background())
					defer iterCancel()
					go utils.IterateOverMap(iterCtx, iterErrorChan, thisMap, iterFunc)
					for err := range iterErrorChan {
						if !testMode || nErrors == 0 {
							errorChan <- err
							nErrors++
						}
					}

					// Sort the items back into an ordered array by block number
					items := make([]*types.SimpleToken, 0, len(thisMap))
					for _, tx := range thisMap {
						items = append(items, tx)
					}
					sort.Slice(items, func(i, j int) bool {
						if opts.Reversed {
							i, j = j, i
						}
						return items[i].BlockNumber < items[j].BlockNumber
					})

					prevBalance, _ = opts.Conn.GetBalanceAt(mon.Address, filter.GetOuterBounds().First)
					for idx, item := range items {
						item := item
						if err := visitToken(idx, item); err != nil {
							errorChan <- err
							return
						}
					}
				}

				bar.Finish(true /* newLine */)
			}
			prevBalance = big.NewInt(0)
		}
	}

	extra := map[string]interface{}{
		"testMode": testMode,
		"export":   true,
		"parts":    []string{"blockNumber", "date", "holder", "balance", "diff", "units"},
	}

	if opts.Globals.Verbose || opts.Globals.Format == "json" {
		parts := names.Custom | names.Prefund | names.Regular
		if namesMap, err := names.LoadNamesMap(chain, parts, nil); err != nil {
			return err
		} else {
			extra["namesMap"] = namesMap
		}
	}

	return output.StreamMany(ctx, fetchData, opts.Globals.OutputOptsWithExtra(extra))
}