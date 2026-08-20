import assert from "node:assert/strict";

// these ids should *never* change (these are bitflags in a u64)
const competitive_bits = {
    ams1: 1,
    gbr1: 2,
    gen1: 3,
    gmec2: 4,
    gsg1: 5,
    gtk1: 6,
    gue4: 7,
    icn1: 8,
    las1: 9,
    ord1: 10,
    syd2: 11,
    tpe1: 12,
};


const blizzard_dacom_kr = "110.45.208.0/24,117.52.6.0/24,117.52.26.0/23,117.52.28.0/23,117.52.33.0/24,117.52.34.0/23,117.52.36.0/23,121.254.137.0/24,121.254.206.0/23,121.254.218.0/24,182.162.31.0/24";

const groups = {
    "blizzard/ord1": "64.224.0.0/21,24.105.40.0/21",
    "blizzard/las1": "64.224.24.0/23",
    "blizzard/icn1": blizzard_dacom_kr,
    "blizzard/syd2": "158.115.196.0/23",
    "blizzard/tpe1": "5.42.160.0/22,5.42.164.0/22",
    "blizzard/ams1": "64.224.26.0/23",
};

// add gpc to groups
{
    const response = await fetch("https://www.gstatic.com/ipranges/cloud.json");
    const data = await response.json();

    Object.assign(groups, Object.fromEntries(Object.entries(
        data.prefixes.reduce((acc, x) => ((acc[`google/${x.scope}`] ??= []).push(x.ipv4Prefix ?? x.ipv6Prefix), acc), {})
    ).map(([scope, prefixes]) => [scope, prefixes.join(',')])));
}

// helpful ~ ^.^ ~
console.table(Object.keys(groups));

const overwatch = [
    // ams1
    {
        title: 'netherlands',
        token: 'ams1',
        block: groups["blizzard/ams1"],
        bit: competitive_bits.ams1,
        ping: "137.221.78.60",
    },
    // gbr1
    {
        title: 'brazil 2',
        token: 'gbr1',
        block: groups["google/southamerica-east1"],
        bit: competitive_bits.gbr1,
        ping: "34.39.128.0",
    },
    // gen1
    {
        title: 'finland 2',
        token: 'gen1',
        block: groups["google/europe-north1"],
        bit: competitive_bits.gen1,
        ping: "34.88.0.1",
    },
    // gmec2
    {
        title: 'saudi arabia',
        token: 'gmec2',
        block: groups["google/me-central2"],
        bit: competitive_bits.gmec2,
        ping: "34.166.0.84",
    },
    // gsg1
    {
        title: 'singapore 2',
        token: 'gsg1',
        block: groups["google/asia-southeast1"],
        bit: competitive_bits.gsg1,
        ping: "34.1.128.4",
    },
    // gtk1
    {
        title: 'japan 2',
        token: 'gtk1',
        block: groups["google/asia-northeast1"],
        bit: competitive_bits.gtk1,
        ping: "34.84.0.0",
    },
    // gue4
    {
        title: 'usa - east 2',
        token: 'gue4',
        block: groups["google/us-east4"],
        bit: competitive_bits.gue4,
        ping: "8.228.65.52",
    },
    // icn1
    // NOTE deleted 08/11/2026, kr has different client now
    // {
    //     title: 'south korea',
    //     token: 'icn1',
    //     block: groups["blizzard/icn1"],
    //     bit: competitive_bits.icn1,
    //     ping: "34.64.64.15",
    // },
    // las1
    {
        title: 'usa - southwest',
        token: 'las1',
        block: groups["blizzard/las1"],
        bit: competitive_bits.las1,
        ping: "34.16.128.42",
    },
    // ord1 
    {
        title: 'usa - central',
        token: 'ord1',
        block: groups["blizzard/ord1"],
        bit: competitive_bits.ord1,
        ping: "8.34.210.23",
    },
    // syd2
    {
        title: 'australia 3',
        token: 'syd2',
        block: groups["blizzard/syd2"],
        bit: competitive_bits.syd2,
        ping: "34.40.128.34",
    },
    // tpe1
    {
        title: 'taiwan',
        token: 'tpe1',
        block: groups["blizzard/tpe1"],
        bit: competitive_bits.tpe1,
        ping: "34.80.0.0",
    },
];

// tests

// bits must be present
assert.ok(
    overwatch.every(x => "bit" in x),
    "missing a bit"
);

// bits must be unique
assert.ok(
    (() => {
        const s = new Set();
        return overwatch.every(x => !s.has(x.bit) && s.add(x.bit));
    })(),
    "duplicate bit found"
);

// bits must map to u64
assert.ok(
    overwatch.every(x => x.bit >= 0 && x.bit < 64 && x.bit % 1) == 0,
    "missing a bit"
);

export {
    overwatch,
};
