import type { Env } from "./env";
import { handleRequest } from "./http/router";
export { KingdomShard } from "./do/KingdomShard";

export default {
  async fetch(request: Request, env: Env, _ctx: ExecutionContext): Promise<Response> {
    return handleRequest(request, env);
  },
};
