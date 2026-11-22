import assert from "node:assert/strict";
import fs from "node:fs";
import { describe, it } from "node:test";

// Set STB_VORBIS_ENTRY=../dist/index.js to test the built bundle instead
const { StbVorbis } = (await import(
    process.env.STB_VORBIS_ENTRY ?? "../src/index.js"
)) as typeof import("../src/index.js");

const read = (name: string) =>
    new Uint8Array(fs.readFileSync(new URL(`files/${name}`, import.meta.url)));

describe("StbVorbis", () => {
    it("decodes a mono stream", async () => {
        await StbVorbis.ready;
        const audio = StbVorbis.decode(read("mono.ogg"));
        assert.equal(audio.channels.length, 1);
        assert.equal(audio.sampleRate, 8000);
        assert.equal(audio.channels[0].length, 2000);
    });

    it("decodes a stereo stream", async () => {
        await StbVorbis.ready;
        const audio = StbVorbis.decode(read("stereo.ogg"));
        assert.equal(audio.channels.length, 2);
        assert.equal(audio.sampleRate, 8000);
        assert.equal(audio.channels[0].length, 2000);
        assert.equal(audio.channels[1].length, 2000);
    });

    it("outputs audible samples within range", async () => {
        await StbVorbis.ready;
        const [channel] = StbVorbis.decode(read("mono.ogg")).channels;
        let peak = 0;
        for (const sample of channel) {
            assert.ok(Number.isFinite(sample));
            peak = Math.max(peak, Math.abs(sample));
        }
        assert.ok(peak > 0.05 && peak <= 1.5, `unexpected peak ${peak}`);
    });

    it("rejects garbage", async () => {
        await StbVorbis.ready;
        assert.throws(() => StbVorbis.decode(new Uint8Array(256).fill(0x41)));
    });

    it("rejects a truncated stream", async () => {
        await StbVorbis.ready;
        const file = read("mono.ogg");
        assert.throws(() => StbVorbis.decode(file.subarray(0, 100)));
    });

    it("keeps working after failures", async () => {
        await StbVorbis.ready;
        assert.throws(() => StbVorbis.decode(new Uint8Array(0)));
        assert.equal(StbVorbis.decode(read("mono.ogg")).channels.length, 1);
    });

    it("survives corrupted input without crashing the process", async () => {
        await StbVorbis.ready;
        const file = read("stereo.ogg");
        for (let seed = 1; seed <= 200; seed++) {
            const copy = file.slice();
            // Cheap deterministic flips, only the outcome matters
            for (let flip = 0; flip < 4; flip++) {
                const index = (seed * 7919 + flip * 104_729) % copy.length;
                copy[index] ^= 1 << ((seed + flip) % 8);
            }
            try {
                StbVorbis.decode(copy);
            } catch (error) {
                assert.ok(error instanceof Error);
            }
        }
        assert.equal(StbVorbis.decode(file).channels.length, 2);
    });
});
