import fs from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

// Dependabot cannot see vendored files, so compare the version by hand
const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const versionPattern = /v(\d+\.\d+)/;

function readVersion(source: string) {
    return versionPattern.exec(source.split("\n", 1)[0])?.[1];
}

const vendored = await fs.readFile(
    path.resolve(root, "wasm/stb_vorbis.c"),
    "utf8"
);
const response = await fetch(
    "https://raw.githubusercontent.com/nothings/stb/master/stb_vorbis.c"
);
if (!response.ok) {
    throw new Error(
        `Could not fetch upstream stb_vorbis.c: ${response.status}`
    );
}

const local = readVersion(vendored);
const upstream = readVersion(await response.text());

if (local === undefined || upstream === undefined) {
    throw new Error("Could not read the version from the stb_vorbis.c header");
}

console.info(`Vendored: v${local}, upstream: v${upstream}`);

if (local !== upstream) {
    throw new Error("The vendored stb_vorbis.c is out of date!");
}
