import { defineConfig } from "vitest/config";

export default defineConfig({
    esbuild: {
        jsx: "automatic",
    },
    test: {
        environment: "node",
        include: [
            "frontend/**/*.test.ts",
            "common/**/*.test.ts",
            "backend/**/*.test.ts",
        ],
        reporters: ["verbose"],
        fileParallelism: false,
        singleFork: true,
        maxConcurrency: 1,
        maxWorkers: 1,
        pool: "forks",
    },
});
