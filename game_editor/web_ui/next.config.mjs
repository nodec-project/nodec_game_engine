const basePath = process.env.NEXT_PUBLIC_BASE_PATH || "";

/** @type {import('next').NextConfig} */
const nextConfig = {
    basePath: basePath,
    output: "export",
    trailingSlash: true,
    compiler: {
        emotion: true
    },
};

export default nextConfig;
