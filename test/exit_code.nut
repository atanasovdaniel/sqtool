switch(vargv[0])
{
    case "integer": return vargv[1].tointeger();
    
    case "throw": throw "hello";
    case "suspend": ::suspend("hello");
    
    default: return 0;
}
