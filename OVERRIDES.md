# Overrides Guide

Dropship supports configurable server IP addresses via `overrides.json` - no recompilation needed!

## How It Works

Create **optional** `overrides.json` next to `dropship.exe` to override built-in IP addresses. Only include entries you want to customize - everything else uses hardcoded defaults.

## Example: Override Brazil Server

Create `overrides.json`:

```json
{
  "servers": {
    "google/southamerica-east1": {
      "block": "34.39.128.0/17,34.95.128.0/17,34.104.80.0/21,34.124.16.0/21,34.151.0.0/18,34.151.192.0/18,35.198.0.0/18,35.199.64.0/18,35.215.192.0/18,35.220.40.0/24,35.235.0.0/20,35.242.40.0/24,35.247.192.0/18,2600:1900:40f0::/44,2600:1902:200::/44"
    }
  },
  "endpoints": {
    "Brazil": {
      "description": "GBR1",
      "ip_ping": "34.39.128.0",
      "blocked_servers": ["google/southamerica-east1"]
    }
  }
}
```

Save and restart Dropship. All other regions use built-in defaults.

## Available Server IDs

- `blizzard/ord1` - USA Central
- `blizzard/las1` - USA West  
- `google/europe-north1` - Finland
- `google/asia-southeast1` - Singapore
- `google/southamerica-east1` - Brazil
- `google/asia-northeast1` - Tokyo
- `google/me-central2` - Saudi Arabia
- `blizzard/icn1` - South Korea
- `blizzard/syd2` - Australia
- `blizzard/tpe1` - Taiwan
- `blizzard/ams1` - Netherlands

## Adding New Regions

Add entries to both `servers` and `endpoints` sections with matching IDs, then restart.

## Notes

- **Optional**: No overrides.json needed - app works with built-in defaults
- **Override**: Only entries in overrides.json replace defaults
- **Fallback**: Invalid JSON? App uses built-in defaults automatically
- **CIDR Format**: Use comma-separated CIDR ranges (e.g., `10.0.0.0/8,192.168.0.0/16`)