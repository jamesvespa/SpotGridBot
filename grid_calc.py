
#!/usr/bin/env python3
import argparse, math, json

def grid_bot_param_calculator(price, range_up_pct, range_down_pct, step_pct, capital, order_size_pct):
    levels_above = math.floor(range_up_pct / step_pct)
    levels_below = math.floor(range_down_pct / step_pct)
    total_orders = levels_above + levels_below
    allocation_per_order = capital * (order_size_pct / 100)

    sell_prices = [round(price * (1 + (i+1) * step_pct / 100), 2) for i in range(levels_above)]
    buy_prices = [round(price * (1 - (i+1) * step_pct / 100), 2) for i in range(levels_below)]

    return {
        "levels_above": levels_above,
        "levels_below": levels_below,
        "total_orders": total_orders,
        "allocation_per_order": allocation_per_order,
        "sell_prices": sell_prices,
        "buy_prices": buy_prices
    }

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Grid Bot Parameter Calculator")
    parser.add_argument("--price", type=float, required=True, help="Current market price")
    parser.add_argument("--range-up", type=float, required=True, help="Range above current price in %")
    parser.add_argument("--range-down", type=float, required=True, help="Range below current price in %")
    parser.add_argument("--step", type=float, required=True, help="Step size between grid levels in %")
    parser.add_argument("--capital", type=float, required=True, help="Total available capital")
    parser.add_argument("--order-size", type=float, required=True, help="Percentage of capital per order")
    parser.add_argument("--json", action="store_true", help="Output as JSON")

    args = parser.parse_args()

    result = grid_bot_param_calculator(
        args.price,
        args.range_up,
        args.range_down,
        args.step,
        args.capital,
        args.order_size
    )

    if args.json:
        print(json.dumps(result, indent=2))
    else:
        print(f"Levels Above: {result['levels_above']}")
        print(f"Levels Below: {result['levels_below']}")
        print(f"Total Orders: {result['total_orders']}")
        print(f"Allocation per Order: ${result['allocation_per_order']:.2f}")
        print(f"Sell Prices: {result['sell_prices']}")
        print(f"Buy Prices: {result['buy_prices']}")
