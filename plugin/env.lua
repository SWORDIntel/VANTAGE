local M = {}

function M.setup(config)
    config.set_environment_variables = {
        SENTINEL_MODE = '1',
    }
end

return M
