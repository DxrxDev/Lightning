#! /usr/bin/lua

c_comp = "gcc"

platforms = {
	invalid = -1,
	linux = 0,
	windows = 1,
	macos = 2
}

warnings = {
	none = 0,
	all = 1
}

actiontypes = {
    run = 0,
    install = 1,
    clear = 2
}

projecttypes = {
    app = 0,
    int = 1,
    sha = 2,
    act = 3
}

local mokosh = {
    stated = false,
    platform = platforms.invalid
}

function MokoshStart( platform )
    if (mokosh.started == true) then
        print "Mokosh already runing.."
        os.exit(-1)
    end
    mokosh.platform = platform
    mokosh.started = true
    print "Mokosh Started.."
end

function CreateProject( name, outdir, type )
	if mokosh.platform ~= platforms.linux then
        print "get gone"
		os.exit(-1)
	end

	if type == projecttypes.app then
		return {
			name = name,
			outdir = outdir,
			program = c_comp,
			special = ""
		}
	elseif type == projecttypes.int then
		return {
			name = name,
			outdir = outdir,
			program = c_comp,
			special = "i"
		}
	elseif type == projecttypes.sha then
		return {
			name = name,
			outdir = outdir,
			program = "glslc",
			special = "s"
		}
	elseif type == projecttypes.act then
		return {
			name = name,
			outdir = outdir, -- outdir here is just the command to be ran
			special = "c"
		}
	end

	print (
		"FAILED CREATING PROJECT\n" ..
		"Project name: " .. name .. "\n" ..
		"Error: Project type \"" .. type .. "\" doesn't exist!\n"
	)
	os.exit(-1)
end

function CreateCommandApp( proj )
    outdir = proj.outdir

	ret = proj.program .. " -o" .. proj.outdir .. "/" .. proj.name .. ".out " .. "-std=c23 "

	numFiles = #proj.files
	for i = 1, numFiles do
		ret = ret .. proj.files[i] .. " "
	end

	if proj.files == nil or proj.files == {} then
		return {ret}
	end

    if (proj.libraries ~= nil) then
	numLibraries = #proj.libraries
	    for i = 1, numLibraries do
		    ret = ret .. "-l" .. proj.libraries[i] .. " "
	    end
    end

	return {ret}
end

function CreateCommandInter( proj )
	ret = {}
	
	if proj.files == nil or proj.files == {} then
		assert(nil, "No files given when building "..proj.name)
	end
	for i = 1, #proj.files do
		ret[i] = 
			proj.program .. " -o" .. 
			proj.outdir .. "/" .. 
			proj.files[i] .. ".o -c " ..
			proj.files[i] .. " " .. "-std=c23 "

		if mokosh.platform == platforms.linux then
			ret[i] = ret[i] .. "-D_PLATFORM_LINUX "
		end

		if proj.warnings ~= nil then
			if proj.warnings == warnings.all then
				ret[i] = ret[i] .. " -Wall "
			end
		end

		if proj.includedirs == nil or proj.includedirs == {} then
		else
			for j = 1, #proj.includedirs do
				ret[i] = ret[i] .. "-I" .. proj.includedirs[j] .. " "
			end
		end
	end

	return ret
end

function CreateCommandShader( proj )
    ret = {}
	for i = 1, #proj.files do
	    ret[i] = proj.program.." -o"..proj.outdir.."/"..proj.files[i]..".spirv "..proj.files[i].." "
	end

	return ret
end
function CreateCommandCommand( proj )
	ret = proj.outdir .. " "

	return {ret}
end

function CreateCommand( proj )
	if proj.outdir == nil then
		proj.outdir = ""
	end

	if proj.special == "" then
		return CreateCommandApp(proj)
	elseif proj.special == "i" then
		return CreateCommandInter(proj)
	elseif proj.special == "s" then
		return CreateCommandShader(proj)
	elseif proj.special == "c" then
		return CreateCommandCommand(proj)
	end

	assert(nil, "Project is invalid")
end


function CreateAction()
    
end

function RunProject( proj )
	run = CreateCommand( proj )
	for i = 1, #run do
		print( "Running command \""..run[i].."\"" )
		os.execute( run[i] )
	end
end

function getobjs(proj)
	ret = {}
	for i = 1, #proj.files do
		ret[i] = proj.outdir .. "/" .. proj.files[i] .. ".o"
	end
	return ret
end

function addobjs(source, target)
	listofobjs = getobjs(source)
	offset = #target.files
	for i = 1, #listofobjs do
		target.files[i+offset] = listofobjs[i]
	end
end

function PrintHelp( buildargs )
    toprint = "Here are all registered build modules:"
	for i = 1, #buildargs do
		toprint = toprint.."\n"..buildargs[i][1].."\t- Builds project(s)"
		for j = 1, #buildargs[i][2] do
			toprint=toprint.." <"..buildargs[i][2][j].name..">"
		end
	end
	print( toprint )
end

function ExecuteArgs( buildargs, args )
    analysingargumentslen = #"Analysing arguments"
    toprint=""
    for i = 1, analysingargumentslen do
    	toprint=toprint.."_"
    end
    toprint=toprint.."\nAnalysing arguments...\n"
    torun = {}
    for i = 1, #arg do
	    foundarg = false
	    for j = 1, #buildargs do
		    if buildargs[j][1] == arg[i] then
			    foundarg = true
			    toprint=toprint.."\n[MODULE BUILD]: "..arg[i].."\n"
			    for k = 1, #buildargs[j][2] do
				    torun[1 + #torun] = buildargs[j][2][k] ; toprint = toprint .."|--("..buildargs[j][2][k].name..")\n"
			    end
		    end
	    end
	    if foundarg == false then
		    print( "ERROR: INVALID ARGUMENT: "..arg[i].."\nABORTING..." )
		    os.exit(-1)
        end
	end

    print(toprint)

    for i = 1, #torun do
	    RunProject(torun[i])
    end
end
