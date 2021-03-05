module.exports = {
    lintOnSave: false,
    configureWebpack: {
        devtool: 'source-map',
    },
    chainWebpack: config => {
        config.plugin('html').tap(args => {
            args[0].title = 'Hivexplore';
            return args;
        });
    },
};
