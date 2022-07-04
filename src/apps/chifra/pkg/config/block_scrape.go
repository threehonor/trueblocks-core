// Copyright 2021 The TrueBlocks Authors. All rights reserved.
// Use of this source code is governed by a license that can
// be found in the LICENSE file.

package config

import (
	"github.com/spf13/viper"
)

// TODO: Multi-chain configure individual tools per chain (for example blockScrape)
var blockScrapeViper = viper.New()
var blockScrapeRead = false
var cachedBlockScrape BlockScrape

type blockScrapeRequires struct {
	Tracing bool
	Parity  bool
	Archive bool
}

// We should remove these underlined config entries in a migration some day
type blockScrapeSettings struct {
	// block_cnt      int
	// block_chan_cnt int
	// addr_chan_cnt  int
	// apps_per_chunk int
	// unripe_dist    int
	// snap_to_grid   int
	// first_snap     int
	// allow_missing  bool
	Allow_missing         bool
	Pinata_api_key        string
	Pinata_secret_api_key string
	Pinata_jwt            string
}

type BlockScrape struct {
	Requires blockScrapeRequires
	Settings blockScrapeSettings
}

// init sets up default values for the given configuration
func init() {
	blockScrapeViper.SetConfigName("blockScrape")
	blockScrapeViper.SetDefault("Requires.Tracing", true)
	blockScrapeViper.SetDefault("Requires.Parity", true)
}

// ReadBlockScrape reads the configuration located in blockScrape.toml file
func ReadBlockScrape(chain string) *BlockScrape {
	if !blockScrapeRead {
		MustReadConfig(blockScrapeViper, &cachedBlockScrape, GetPathToChainConfig(chain))
		blockScrapeRead = true
	}

	return &cachedBlockScrape
}
