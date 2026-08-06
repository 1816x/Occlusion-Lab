import nextVitals from "eslint-config-next/core-web-vitals";
import nextTypescript from "eslint-config-next/typescript";

const eslintConfig = [
  ...nextVitals,
  ...nextTypescript,
  { ignores: [".next/**", "next-env.d.ts", "node_modules/**", "public/draco/**"] },
];

export default eslintConfig;
