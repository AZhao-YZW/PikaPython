use crate::compiler::*;
use crate::version_info::*;
use std::fs::File;
use std::io::prelude::*;

pub fn pika_compiler_entry() {
    /* new a version_info object */
    println!("(pikascript) packages installed:");
    let mut version_info = VersionInfo::new();
    version_info = VersionInfo::analyse_file(version_info, String::from("requestment.txt"));
    println!();

    println!("(pikascript) pika compiler:");
    /* new a compiler, sellect to path */
    let mut compiler = Compiler::new(String::from(""), String::from("pikascript-api/"));

    /* analyse file begin with main.py */
    compiler = Compiler::analyse_inner_package(compiler, String::from("main"));
    /*
       Compile packages in requestment.txt, solve the packages
       as the top packages.
    */
    for package in &version_info.package_list {
        /* skip pikascript-core */
        if package.0 == "pikascript-core" {
            continue;
        }
        compiler = Compiler::analyse_top_package(compiler, String::from(package.0));
    }

    /* Compile packages in PikaStdLib */
    compiler = Compiler::analyse_top_package(compiler, String::from("PikaStdTask"));
    compiler = Compiler::analyse_top_package(compiler, String::from("PikaStdData"));
    compiler = Compiler::analyse_top_package(compiler, String::from("PikaDebug"));

    // println!();

    /* write the infomatrion to compiler-info */
    let mut compiler_info_file =
        File::create(format!("{}compiler-info.txt", compiler.dist_path)).unwrap();
    let compiler_info = format!("{:?}", compiler);
    compiler_info_file.write(compiler_info.as_bytes()).unwrap();

    /* make the -api.c file for each python class */
    for (_, class_info) in compiler.class_list.iter() {
        let api_file_path = format!("{}{}-api.c", compiler.dist_path, class_info.this_class_name);
        let mut f = File::create(api_file_path).unwrap();
        f.write("/* ******************************** */\n".as_bytes())
            .unwrap();
        f.write("/* Warning! Don't modify this file! */\n".as_bytes())
            .unwrap();
        f.write("/* ******************************** */\n".as_bytes())
            .unwrap();
        /* create include for calsses */
        f.write(class_info.include().as_bytes()).unwrap();
        f.write("#include <stdio.h>\n".as_bytes()).unwrap();
        f.write("#include <stdlib.h>\n".as_bytes()).unwrap();
        f.write("#include \"BaseObj.h\"\n".as_bytes()).unwrap();
        f.write("\n".as_bytes()).unwrap();
        /* create method api function */
        f.write(class_info.method_api_fn().as_bytes()).unwrap();
        /* create new classs function */
        f.write(class_info.new_class_fn().as_bytes()).unwrap();
        f.write("\n".as_bytes()).unwrap();
        /* create contruactor */
        if !class_info.is_package {
            let name = String::from(class_info.this_class_name.to_string());
            f.write(format!("Arg *{}(PikaObj *self){{\n", &name).as_bytes())
                .unwrap();
            f.write(format!("    return obj_newObjInPackage(New_{});\n", &name).as_bytes())
                .unwrap();
            f.write("}\n".as_bytes()).unwrap();
        }
    }

    /* make the .h file for each python class */
    for (_, class_info) in compiler.class_list.iter() {
        let api_file_path = format!("{}{}.h", compiler.dist_path, class_info.this_class_name);
        let mut f = File::create(api_file_path).unwrap();
        f.write("/* ******************************** */\n".as_bytes())
            .unwrap();
        f.write("/* Warning! Don't modify this file! */\n".as_bytes())
            .unwrap();
        f.write("/* ******************************** */\n".as_bytes())
            .unwrap();
        f.write(format!("#ifndef __{}__H\n", class_info.this_class_name).as_bytes())
            .unwrap();
        f.write(format!("#define __{}__H\n", class_info.this_class_name).as_bytes())
            .unwrap();
        f.write("#include <stdio.h>\n".as_bytes()).unwrap();
        f.write("#include <stdlib.h>\n".as_bytes()).unwrap();
        f.write("#include \"PikaObj.h\"\n".as_bytes()).unwrap();
        f.write("\n".as_bytes()).unwrap();
        let new_class_fn_declear = format!("{};\n", class_info.new_class_fn_name());
        f.write(new_class_fn_declear.as_bytes()).unwrap();
        f.write("\n".as_bytes()).unwrap();
        f.write(class_info.method_impl_declear().as_bytes())
            .unwrap();
        f.write("\n".as_bytes()).unwrap();
        f.write("#endif\n".as_bytes()).unwrap();
    }
    /* make the pikascript.c */
    let api_file_path = format!("{}pikaScript.c", compiler.dist_path);
    let mut f = File::create(api_file_path).unwrap();
    /* add head */
    f.write("/* ******************************** */\n".as_bytes())
        .unwrap();
    f.write("/* Warning! Don't modify this file! */\n".as_bytes())
        .unwrap();
    f.write("/* ******************************** */\n".as_bytes())
        .unwrap();
    /* add include */
    f.write("#include \"PikaMain.h\"\n".as_bytes()).unwrap();
    f.write("#include <stdio.h>\n".as_bytes()).unwrap();
    f.write("#include <stdlib.h>\n".as_bytes()).unwrap();
    f.write("\n".as_bytes()).unwrap();
    /* get script from main.py */
    let pika_main = compiler
        .class_list
        .get_mut(&"PikaMain".to_string())
        .unwrap();
    /* make the pikascript.c */
    f.write(pika_main.script_fn(version_info).as_bytes())
        .unwrap();

    /* make the pikascript.h */
    let api_file_path = format!("{}pikaScript.h", compiler.dist_path);
    let mut f = File::create(api_file_path).unwrap();
    f.write("/* ******************************** */\n".as_bytes())
        .unwrap();
    f.write("/* Warning! Don't modify this file! */\n".as_bytes())
        .unwrap();
    f.write("/* ******************************** */\n".as_bytes())
        .unwrap();
    f.write(format!("#ifndef __{}__H\n", "pikaScript").as_bytes())
        .unwrap();
    f.write(format!("#define __{}__H\n", "pikaScript").as_bytes())
        .unwrap();
    f.write("#include <stdio.h>\n".as_bytes()).unwrap();
    f.write("#include <stdlib.h>\n".as_bytes()).unwrap();
    f.write("#include \"PikaObj.h\"\n".as_bytes()).unwrap();
    f.write("#include \"PikaMain.h\"\n".as_bytes()).unwrap();
    f.write("\n".as_bytes()).unwrap();
    f.write("PikaObj * pikaScriptInit(void);\n".as_bytes())
        .unwrap();
    f.write("\n".as_bytes()).unwrap();
    f.write("#endif\n".as_bytes()).unwrap();
}
