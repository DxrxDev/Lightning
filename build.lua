#! /usr/bin/lua

require "Mokosh"

MokoshStart( platforms.linux )

wingfx = CreateProject( "wingfx", "out/int", projecttypes.int )
wingfx.files = {
	"Lightning/wingfx.c"
}
wingfx.includedirs = {
	"MasterMediaManager"
}
wingfx.warnings = warnings.all

logic = CreateProject( "logix", "out/int", projecttypes.int )
logic.files = {
	"Lightning/logic.c"
}
logic.warnings = warnings.all

geom = CreateProject( "geom", "out/int", projecttypes.int )
geom.files = {
	"Lightning/geometry.c"
}
geom.warnings = warnings.all

app = CreateProject( "app", "out/int", projecttypes.int )
app.files = {
	"Project/main.c",
}
app.includedirs = {
	"Lightning", "MasterMediaManager"
}
app.warnings = warnings.all

game = CreateProject( "StonesToBridges", "out", projecttypes.app )
game.files = {
	"out/int/Project/main.c.o"
}
addobjs( wingfx, game )
addobjs( logic, game )
addobjs( geom, game )

game.libraries = {
	"dl", "m", "xcb", "xcb-icccm", "xcb-keysyms", "vulkan"
}
game.warnings = warnings.all

shaders = CreateProject( "vert", "out", projecttypes.sha )
shaders.files = {"world.vert", "world.frag", "ui.vert", "ui.frag", "land.vert", "land.frag" }

rungame = CreateProject( "Run Game", "cd "..game.outdir.."/ ; ./"..game.name..".out", projecttypes.act )

buildargs = {
	{"g",  {game}},
	{"wg", {wingfx}},
	{"l",  {logic}},
    {"gm", {geom}},
	{"a",  {app}},
	{"s",  {shaders}},
	{"all", {wingfx, logic, geom, app, shaders, game}},

	{"run", {rungame}}
}

if #arg == 0 then
	PrintHelp( buildargs )
	os.exit(0)
else
    ExecuteArgs( buildargs, args )
end
