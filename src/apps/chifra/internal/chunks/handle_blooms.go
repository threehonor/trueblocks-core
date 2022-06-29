// Copyright 2021 The TrueBlocks Authors. All rights reserved.
// Use of this source code is governed by a license that can
// be found in the LICENSE file.

package chunksPkg

import (
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/cache"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/index"
	"github.com/TrueBlocks/trueblocks-core/src/apps/chifra/pkg/types"
)

func (opts *ChunksOptions) showBloom(path string, data *interface{}, first bool) (bool, error) {
	var bloom index.ChunkBloom
	index.ReadBloom(&bloom, path)

	if opts.Globals.Verbose {
		bloom.Display(int(opts.Globals.LogLevel))
	}

	// TODO: Fix export without arrays
	stats := NewChunkStats(path)
	obj := NewSimpleBloom(stats, bloom)
	err := opts.Globals.RenderObject(obj, first)
	if err != nil {
		return false, err
	}
	return true, nil
}

func NewSimpleBloom(stats types.SimpleChunkStats, bloom index.ChunkBloom) types.SimpleBloom {
	nInserted := 0
	for _, bl := range bloom.Blooms {
		nInserted += int(bl.NInserted)
	}

	var ret types.SimpleBloom
	ret.Size = stats.BloomSz
	ret.Range = cache.FileRange{First: stats.Start, Last: stats.End}
	ret.Count = stats.NBlooms
	ret.Width = index.BLOOM_WIDTH_IN_BYTES
	ret.NInserted = uint64(nInserted)

	return ret
}

func (opts *ChunksOptions) HandleBlooms(blockNums []uint64) error {
	maxTestItems = 10

	defer opts.Globals.RenderFooter()
	err := opts.Globals.RenderHeader(types.SimpleBloom{}, &opts.Globals.Writer, opts.Globals.Format, opts.Globals.ApiMode, opts.Globals.NoHeader, true)
	if err != nil {
		return err
	}

	ctx := WalkContext{
		VisitFunc: opts.showBloom,
	}
	return opts.WalkIndexFiles(cache.Index_Bloom, ctx, blockNums)
}
