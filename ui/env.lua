local M = {}

function M.setup(config)
    config.set_environment_variables = {
        VANTAGE_MODE = '1',
    }
end

return M
