import { defineConfig } from "vitest/config";

export default defineConfig({
    resolve: {
        conditions: ["development"],
    },
    test: {
        environment: "node",
        include: ["tests/**/*.test.ts"],
        reporters: ["verbose"],
        fileParallelism: false,
        singleFork: true,
        maxConcurrency: 1,
        maxWorkers: 1,
        pool: "forks",
        testTimeout: 10_000,
        hookTimeout: 10_000,
        teardownTimeout: 15_000,
    },
});
