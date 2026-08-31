import { defineConfig } from "vitest/config";

export default defineConfig({
    test: {
        environment: "node",
        include: ["tests/**/*.test.ts"],
        reporters: ["verbose"],
        fileParallelism: false,
        maxConcurrency: 1,
        maxWorkers: 1,
        pool: "forks",
        testTimeout: 10_000,
        hookTimeout: 10_000,
        teardownTimeout: 15_000,
    },
});
