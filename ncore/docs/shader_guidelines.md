Some Slang shading language best practices, taken from various Slang docs:
- Avoid Manual Annotations: use ParameterBlock<> types instead
- Avoid Polluting the Global Scope: use Entry-Point parameters in Computer Shaders.
- When multiple parameter blocks are used, developers are advised to declare blocks that are expected to change less frequently before those that will change more frequently.
