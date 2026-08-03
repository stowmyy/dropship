import { writeFile } from "node:fs/promises";

import { notices } from "./assets/notices.js";

import { overwatch } from "./ips/overwatch.js";

const data = {
    notices,
    servers: {
        overwatch,
    },
};

console.dir(data, { depth: 99 });
await writeFile("./output/ips.json", JSON.stringify(data, null, 2));
