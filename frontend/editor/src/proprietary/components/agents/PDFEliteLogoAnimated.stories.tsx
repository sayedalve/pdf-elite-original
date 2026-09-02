import type { Meta, StoryObj } from "@storybook/react-vite";
import { PDFEliteLogoAnimated } from "@app/components/agents/PDFEliteLogoAnimated";

/**
 * Animated PDFElite logo mark, used as a "thinking" indicator in the chat panel.
 */
const meta: Meta<typeof PDFEliteLogoAnimated> = {
  title: "Agents/PDFEliteLogoAnimated",
  component: PDFEliteLogoAnimated,
  parameters: { layout: "padded" },
  args: {
    size: 20,
  },
};
export default meta;
type Story = StoryObj<typeof meta>;

export const Default: Story = {};

export const Large: Story = {
  args: { size: 64 },
};
