## chifra chunks

The chifra chunks routine provides tools for interacting with, checking the validity of,
cleaning up, and analyizing the Unchained Index. It provides options to list pins,
the Manifest, summary data on the index, Bloom filters, addresses, and appearances.
While still in its early stages, this tool will eventually allow users to clean
their local index, clean their remote index, study the indexes, etc. Stay tuned.

```[plaintext]
Purpose:
  Manage, investigate, and display the Unchained Index.

Usage:
  chifra chunks <mode> [flags] [blocks...] [address...]

Arguments:
  mode - the type of data to process (required)
	One of [ status | manifest | index | blooms | addresses | appearances | stats ]
  blocks - an optional list of blocks to intersect with chunk ranges

Flags:
  -c, --check             check the manifest, index, or blooms for internal consistency
  -i, --pin               pin the manifest or each index chunk and bloom
  -p, --publish           publish the manifest to the Unchained Index smart contract
  -n, --truncate uint     truncate the entire index at this block (requires a block identifier)
  -m, --remote            prior to processing, retreive the manifest from the Unchained Index smart contract
  -b, --belongs strings   in index mode only, checks the address(es) for inclusion in the given index chunk
  -s, --sleep float       for --remote pinning only, seconds to sleep between API calls
  -x, --fmt string        export format, one of [none|json*|txt|csv]
  -v, --verbose           enable verbose (increase detail with --log_level)
  -h, --help              display this help screen

Notes:
  - Mode determines which type of data to display or process.
  - Certain options are only available in certain modes.
  - If blocks are provided, only chunks intersecting with those blocks are displayed.
  - The --truncate option updates data, but does not --pin or --publish.
  - You may combine the --pin and --publish options.
  - The --belongs option is only available in the index mode.
```

**Source code**: [`internal/chunks`](https://github.com/TrueBlocks/trueblocks-core/tree/master/src/apps/chifra/internal/chunks)
