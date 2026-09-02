import type { Meta, StoryObj } from "@storybook/react-vite";
import RemovePagesSettings from "@app/components/tools/removePages/RemovePagesSettings";
import { RemovePagesParameters } from "@app/hooks/tools/removePages/useRemovePagesParameters";

const meta = {
  title: "Tools/RemovePages/RemovePagesSettings",
  component: RemovePagesSettings,
} satisfies Meta<typeof RemovePagesSettings>;
export default meta;

type Story = StoryObj<typeof meta>;

const baseParameters: RemovePagesParameters = {
  pageNumbers: "",
};

export const Default: Story = {
  args: {
    parameters: baseParameters,
    onChange: () => {},
  },
};

export const FilledValid: Story = {
  args: {
    parameters: { pageNumbers: "1,3,5-8,10" },
    onChange: () => {},
  },
};

export const InvalidInput: Story = {
  args: {
    parameters: { pageNumbers: "abc" },
    onChange: () => {},
  },
};

export const Disabled: Story = {
  args: {
    parameters: baseParameters,
    onChange: () => {},
    disabled: true,
  },
};
