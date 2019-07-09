// run this script directly from the command line
const { Hang, testAsync } = require('./build/Release/uv_async_hang');

// let's hang async/await operations
let hang = new Hang();

function doTestAsync() {
    return new Promise((resolve, reject) => {
        testAsync((data) => {
            console.log(data);
            resolve(data);
        })
    });
}

// this is ok
//testAsync((data) => {
//    console.log(data);
//});

async function testHang() {
    let asyncRet = await doTestAsync();

    // async/await operation blocks due to the creation of "hang"
    console.log("after async call");
}


testHang();

