from pathlib import Path
from cutekit import cli, model, builder, shell, const


@cli.command("d", "dist", "Make booboot easier to distribute")
def _(args: model.TargetArgs):
    registry = model.Registry.use(args)
    component = registry.lookup("booboot", model.Component)
    dist = Path(const.PROJECT_CK_DIR) / "dist"
    dist.mkdir(parents=True, exist_ok=True)

    for target in registry.iter(model.Target):
        scope = builder.TargetScope(registry, target)
        shell.cp(
            builder.build(scope, component)[0].path,
            str(dist / target.props["filename"]),
        )
