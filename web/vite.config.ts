import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// Relative base so the build runs from any GH Pages sub-path
// (project pages serve from /<repo-name>/).
export default defineConfig({
  base: "./",
  plugins: [react()],
});
