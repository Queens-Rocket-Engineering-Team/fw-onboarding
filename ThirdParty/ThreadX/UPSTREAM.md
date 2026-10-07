# Eclipse ThreadX source provenance

- Repository: https://github.com/eclipse-threadx/threadx
- Release: `v6.5.2.202603_rel`
- Commit: `93387b0a60382a29579bbad5045c68328e3751df`
- Imported directories: `common/`, `ports/cortex_m3/gnu/`
- License: upstream `LICENSE.txt` (MIT)

The imported source files are unmodified. Only the common kernel and Cortex-M3
GNU port are built; upstream example startup, sample applications, and linker
scripts are not part of the firmware. Board integration lives in the project's
`ThreadX/` directory.

This directory deliberately lives outside `Middlewares/`, which CubeMX can
remove during generation when middleware is not registered in the `.ioc` file.

Update both directories together from a tagged upstream release and update this
record. Rebuild Debug and Release, check interrupt ownership and memory usage,
then repeat the board verification described in the root README.
