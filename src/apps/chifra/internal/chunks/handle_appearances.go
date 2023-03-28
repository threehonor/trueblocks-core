// Copyright 2021 The TrueBlocks Authors. All rights reserved.
// Use of this source code is governed by a license that can
// be found in the LICENSE file.

package chunksPkg

import (
	"context"
	"fmt"
	"io"

	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/cache"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/file"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/index"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/output"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/types"
)

func (opts *ChunksOptions) HandleAppearances(blockNums []uint64) error {
	ctx, cancel := context.WithCancel(context.Background())
	fetchData := func(modelChan chan types.Modeler[types.RawAppearance], errorChan chan error) {
		showAppearances := func(walker *index.IndexWalker, path string, first bool) (bool, error) {
			if path != cache.ToBloomPath(path) {
				err := fmt.Errorf("should not happen in showAppearances")
				errorChan <- err
				cancel()
				return false, nil
			}

			path = cache.ToIndexPath(path)
			if !file.FileExists(path) {
				// Bloom files exist, but index files don't. It's okay.
				return true, nil
			}

			indexChunk, err := index.NewChunkData(path)
			if err != nil {
				return false, err
			}
			defer indexChunk.Close()

			_, err = indexChunk.File.Seek(indexChunk.AppTableStart, io.SeekStart)
			if err != nil {
				errorChan <- err
				cancel()
				return false, nil
			}

			for i := 0; i < int(indexChunk.Header.AppearanceCount); i++ {
				if opts.Globals.TestMode && i > walker.MaxTests() {
					continue
				}
				rec := index.AppearanceRecord{}
				err := rec.ReadAppearance(indexChunk.File)
				if err != nil {
					errorChan <- err
					return false, nil
				}
				s := types.SimpleAppearance{
					BlockNumber:      rec.BlockNumber,
					TransactionIndex: rec.TransactionId,
				}
				modelChan <- &s
			}

			return true, nil
		}

		walker := index.NewIndexWalker(
			opts.Globals.Chain,
			opts.Globals.TestMode,
			10, /* maxTests */
			showAppearances,
		)

		if err := walker.WalkBloomFilters(blockNums); err != nil {
			errorChan <- err
		}
	}

	return output.StreamMany(ctx, fetchData, output.OutputOptions{
		Writer:     opts.Globals.Writer,
		Chain:      opts.Globals.Chain,
		TestMode:   opts.Globals.TestMode,
		NoHeader:   opts.Globals.NoHeader,
		ShowRaw:    opts.Globals.ShowRaw,
		Verbose:    opts.Globals.Verbose,
		LogLevel:   opts.Globals.LogLevel,
		Format:     opts.Globals.Format,
		OutputFn:   opts.Globals.OutputFn,
		Append:     opts.Globals.Append,
		JsonIndent: "  ",
		Extra: map[string]any{
			"appearances": true,
		},
	})
}
