import type { Metadata } from "next";
import "./globals.css";

export const metadata: Metadata = {
  title: "Occlusion Lab",
  description: "Educational occlusion simulation foundation sandbox.",
};

export default function RootLayout({ children }: Readonly<{ children: React.ReactNode }>) {
  return (
    <html lang="en">
      <body>{children}</body>
    </html>
  );
}
