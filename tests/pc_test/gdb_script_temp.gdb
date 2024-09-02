
    #connect to remote server
    target remote :1234

    # Set a breakpoint at the specific address
    tb bist_verify_pc_test
    continue

    # Commands to run when breakpoint is hit
    commands
        set pcTestFunctions[0]=(pcTestFunctions[0]-4)
        continue
    end
    